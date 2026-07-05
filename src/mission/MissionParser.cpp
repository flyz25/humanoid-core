#include <humanoid/mission/MissionParser.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <humanoid/core/CommandPriority.h>
#include <humanoid/core/CommandType.h>

namespace humanoid::mission {
namespace {

class ParseError final : public std::runtime_error {
public:
  explicit ParseError(const std::string& message) : std::runtime_error{message} {}
};

struct JsonValue final {
  using Object = std::map<std::string, JsonValue, std::less<>>;
  using Array = std::vector<JsonValue>;
  using Storage = std::variant<std::nullptr_t, bool, double, std::string, Object, Array>;

  Storage storage{nullptr};
};

[[nodiscard]] MissionParseResult Success(Mission mission) {
  MissionParseResult result;
  result.mission = std::move(mission);
  result.success = true;
  result.message = "Mission parsed successfully";
  return result;
}

[[nodiscard]] MissionParseResult Failure(std::string message) {
  MissionParseResult result;
  result.success = false;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsSpace(char value) noexcept {
  return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

[[nodiscard]] std::string Trim(std::string_view text) {
  std::size_t first = 0U;
  while (first < text.size() && (text[first] == ' ' || text[first] == '\r' || text[first] == '\n' ||
                                 text[first] == '\t')) {
    ++first;
  }

  std::size_t last = text.size();
  while (last > first && (text[last - 1U] == ' ' || text[last - 1U] == '\r' ||
                          text[last - 1U] == '\n' || text[last - 1U] == '\t')) {
    --last;
  }

  return std::string{text.substr(first, last - first)};
}

[[nodiscard]] std::optional<core::CommandType> CommandTypeFromString(std::string_view value) {
  if (value == "Stand") {
    return core::CommandType::Stand;
  }
  if (value == "Sit") {
    return core::CommandType::Sit;
  }
  if (value == "Walk") {
    return core::CommandType::Walk;
  }
  if (value == "Stop") {
    return core::CommandType::Stop;
  }
  if (value == "Move") {
    return core::CommandType::Move;
  }
  if (value == "Rotate") {
    return core::CommandType::Rotate;
  }
  if (value == "HandOpen") {
    return core::CommandType::HandOpen;
  }
  if (value == "HandClose") {
    return core::CommandType::HandClose;
  }
  if (value == "PlayAudio") {
    return core::CommandType::PlayAudio;
  }
  if (value == "StopAudio") {
    return core::CommandType::StopAudio;
  }
  if (value == "Custom") {
    return core::CommandType::Custom;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<core::CommandPriority> PriorityFromString(std::string_view value) {
  if (value == "Low") {
    return core::CommandPriority::Low;
  }
  if (value == "Normal") {
    return core::CommandPriority::Normal;
  }
  if (value == "High") {
    return core::CommandPriority::High;
  }
  if (value == "Critical") {
    return core::CommandPriority::Critical;
  }
  return std::nullopt;
}

[[nodiscard]] std::uint64_t ParseUint64(std::string_view text, std::string_view field_name) {
  const std::string trimmed = Trim(text);
  std::uint64_t value = 0U;
  const char* begin = trimmed.data();
  const char* end = trimmed.data() + trimmed.size();
  const std::from_chars_result result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw ParseError{std::string{field_name} + " must be an unsigned integer"};
  }
  return value;
}

[[nodiscard]] std::int64_t ParseInt64(std::string_view text, std::string_view field_name) {
  const std::string trimmed = Trim(text);
  std::int64_t value = 0;
  const char* begin = trimmed.data();
  const char* end = trimmed.data() + trimmed.size();
  const std::from_chars_result result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw ParseError{std::string{field_name} + " must be an integer"};
  }
  return value;
}

[[nodiscard]] double ParseDouble(std::string_view text, std::string_view field_name) {
  const std::string trimmed = Trim(text);
  double value = 0.0;
  const char* begin = trimmed.data();
  const char* end = trimmed.data() + trimmed.size();
  const std::from_chars_result result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
    throw ParseError{std::string{field_name} + " must be a finite number"};
  }
  return value;
}

[[nodiscard]] bool IsInteger(double value) noexcept {
  double integer_part = 0.0;
  return std::isfinite(value) && std::modf(value, &integer_part) == 0.0 &&
         value >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
         value <= static_cast<double>(std::numeric_limits<std::int64_t>::max());
}

[[nodiscard]] core::CommandPayloadValue PayloadValueFromNumber(double value) {
  if (IsInteger(value)) {
    return static_cast<std::int64_t>(value);
  }
  return value;
}

class JsonParser final {
public:
  explicit JsonParser(std::string_view document) : document_(document) {}

  [[nodiscard]] JsonValue Parse() {
    JsonValue value = ParseValue();
    SkipWhitespace();
    if (position_ != document_.size()) {
      throw ParseError{"Unexpected trailing content in JSON document"};
    }
    return value;
  }

private:
  [[nodiscard]] JsonValue ParseValue() {
    SkipWhitespace();
    if (position_ >= document_.size()) {
      throw ParseError{"Unexpected end of JSON document"};
    }

    const char current = document_[position_];
    if (current == '{') {
      return JsonValue{ParseObject()};
    }
    if (current == '[') {
      return JsonValue{ParseArray()};
    }
    if (current == '"') {
      return JsonValue{ParseString()};
    }
    if (current == '-' || (current >= '0' && current <= '9')) {
      return JsonValue{ParseNumber()};
    }
    if (ConsumeLiteral("true")) {
      return JsonValue{true};
    }
    if (ConsumeLiteral("false")) {
      return JsonValue{false};
    }
    if (ConsumeLiteral("null")) {
      return JsonValue{nullptr};
    }

    throw ParseError{"Unexpected JSON value"};
  }

  [[nodiscard]] JsonValue::Object ParseObject() {
    Expect('{');
    JsonValue::Object object;
    SkipWhitespace();
    if (TryConsume('}')) {
      return object;
    }

    while (true) {
      SkipWhitespace();
      if (position_ >= document_.size() || document_[position_] != '"') {
        throw ParseError{"JSON object keys must be strings"};
      }
      std::string key = ParseString();
      SkipWhitespace();
      Expect(':');
      auto insert_result = object.emplace(std::move(key), ParseValue());
      if (!insert_result.second) {
        throw ParseError{"Duplicate JSON object key"};
      }

      SkipWhitespace();
      if (TryConsume('}')) {
        return object;
      }
      Expect(',');
    }
  }

  [[nodiscard]] JsonValue::Array ParseArray() {
    Expect('[');
    JsonValue::Array array;
    SkipWhitespace();
    if (TryConsume(']')) {
      return array;
    }

    while (true) {
      array.push_back(ParseValue());
      SkipWhitespace();
      if (TryConsume(']')) {
        return array;
      }
      Expect(',');
    }
  }

  [[nodiscard]] std::string ParseString() {
    Expect('"');
    std::string result;
    while (position_ < document_.size()) {
      const char current = document_[position_++];
      if (current == '"') {
        return result;
      }
      if (static_cast<unsigned char>(current) < 0x20U) {
        throw ParseError{"JSON strings must not contain unescaped control characters"};
      }
      if (current != '\\') {
        result.push_back(current);
        continue;
      }

      if (position_ >= document_.size()) {
        throw ParseError{"Unterminated JSON string escape"};
      }
      const char escaped = document_[position_++];
      switch (escaped) {
      case '"':
      case '\\':
      case '/':
        result.push_back(escaped);
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'u':
        throw ParseError{"JSON unicode escapes are not supported in mission documents"};
      default:
        throw ParseError{"Invalid JSON string escape"};
      }
    }

    throw ParseError{"Unterminated JSON string"};
  }

  [[nodiscard]] double ParseNumber() {
    const std::size_t begin = position_;
    if (document_[position_] == '-') {
      ++position_;
    }
    ConsumeDigits();
    if (position_ < document_.size() && document_[position_] == '.') {
      ++position_;
      ConsumeDigits();
    }
    if (position_ < document_.size() &&
        (document_[position_] == 'e' || document_[position_] == 'E')) {
      ++position_;
      if (position_ < document_.size() &&
          (document_[position_] == '+' || document_[position_] == '-')) {
        ++position_;
      }
      ConsumeDigits();
    }

    return ParseDouble(document_.substr(begin, position_ - begin), "JSON number");
  }

  void ConsumeDigits() {
    const std::size_t begin = position_;
    while (position_ < document_.size() && document_[position_] >= '0' &&
           document_[position_] <= '9') {
      ++position_;
    }
    if (begin == position_) {
      throw ParseError{"JSON number requires digits"};
    }
  }

  void SkipWhitespace() {
    while (position_ < document_.size() && IsSpace(document_[position_])) {
      ++position_;
    }
  }

  [[nodiscard]] bool ConsumeLiteral(std::string_view literal) {
    if (document_.substr(position_, literal.size()) != literal) {
      return false;
    }
    position_ += literal.size();
    return true;
  }

  void Expect(char expected) {
    SkipWhitespace();
    if (position_ >= document_.size() || document_[position_] != expected) {
      throw ParseError{std::string{"Expected '"} + expected + "' in JSON document"};
    }
    ++position_;
  }

  [[nodiscard]] bool TryConsume(char expected) {
    SkipWhitespace();
    if (position_ < document_.size() && document_[position_] == expected) {
      ++position_;
      return true;
    }
    return false;
  }

  std::string_view document_;
  std::size_t position_{0U};
};

[[nodiscard]] const JsonValue::Object& AsObject(const JsonValue& value, std::string_view name) {
  if (!std::holds_alternative<JsonValue::Object>(value.storage)) {
    throw ParseError{std::string{name} + " must be an object"};
  }
  return std::get<JsonValue::Object>(value.storage);
}

[[nodiscard]] const JsonValue::Array& AsArray(const JsonValue& value, std::string_view name) {
  if (!std::holds_alternative<JsonValue::Array>(value.storage)) {
    throw ParseError{std::string{name} + " must be an array"};
  }
  return std::get<JsonValue::Array>(value.storage);
}

[[nodiscard]] const JsonValue& Required(const JsonValue::Object& object, std::string_view key) {
  const auto value = object.find(key);
  if (value == object.end()) {
    throw ParseError{"Missing required field: " + std::string{key}};
  }
  return value->second;
}

[[nodiscard]] const JsonValue* Optional(const JsonValue::Object& object, std::string_view key) {
  const auto value = object.find(key);
  if (value == object.end()) {
    return nullptr;
  }
  return &value->second;
}

[[nodiscard]] std::string JsonString(const JsonValue& value, std::string_view key) {
  if (!std::holds_alternative<std::string>(value.storage)) {
    throw ParseError{std::string{key} + " must be a string"};
  }
  return std::get<std::string>(value.storage);
}

[[nodiscard]] std::uint64_t JsonUint64(const JsonValue& value, std::string_view key) {
  if (!std::holds_alternative<double>(value.storage)) {
    throw ParseError{std::string{key} + " must be an unsigned integer"};
  }
  const double number = std::get<double>(value.storage);
  if (!IsInteger(number) || number < 0.0 ||
      number > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    throw ParseError{std::string{key} + " must be an unsigned integer"};
  }
  return static_cast<std::uint64_t>(number);
}

[[nodiscard]] std::uint32_t JsonUint32(const JsonValue& value, std::string_view key) {
  const std::uint64_t number = JsonUint64(value, key);
  if (number > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
    throw ParseError{std::string{key} + " exceeds uint32 range"};
  }
  return static_cast<std::uint32_t>(number);
}

[[nodiscard]] std::int64_t JsonInt64(const JsonValue& value, std::string_view key) {
  if (!std::holds_alternative<double>(value.storage) ||
      !IsInteger(std::get<double>(value.storage))) {
    throw ParseError{std::string{key} + " must be an integer"};
  }
  return static_cast<std::int64_t>(std::get<double>(value.storage));
}

[[nodiscard]] bool JsonBool(const JsonValue& value, std::string_view key) {
  if (!std::holds_alternative<bool>(value.storage)) {
    throw ParseError{std::string{key} + " must be a boolean"};
  }
  return std::get<bool>(value.storage);
}

[[nodiscard]] MissionMetadata MetadataFromJson(const JsonValue& value, std::string_view key) {
  MissionMetadata metadata;
  for (const auto& [metadata_key, metadata_value] : AsObject(value, key)) {
    metadata.emplace(metadata_key, JsonString(metadata_value, key));
  }
  return metadata;
}

[[nodiscard]] core::CommandPayload PayloadFromJson(const JsonValue& value, std::string_view key) {
  core::CommandPayload payload;
  for (const auto& [payload_key, payload_value] : AsObject(value, key)) {
    if (std::holds_alternative<bool>(payload_value.storage)) {
      payload.emplace(payload_key, std::get<bool>(payload_value.storage));
    } else if (std::holds_alternative<double>(payload_value.storage)) {
      payload.emplace(payload_key, PayloadValueFromNumber(std::get<double>(payload_value.storage)));
    } else if (std::holds_alternative<std::string>(payload_value.storage)) {
      payload.emplace(payload_key, std::get<std::string>(payload_value.storage));
    } else {
      throw ParseError{"Command payload values must be scalar"};
    }
  }
  return payload;
}

[[nodiscard]] core::Command CommandFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "command");
  core::Command command;
  command.id = JsonUint64(Required(object, "id"), "command.id");

  const std::string type = JsonString(Required(object, "type"), "command.type");
  const std::optional<core::CommandType> command_type = CommandTypeFromString(type);
  if (!command_type.has_value()) {
    throw ParseError{"Unknown command type: " + type};
  }
  command.type = *command_type;

  if (const JsonValue* priority = Optional(object, "priority")) {
    const std::string priority_name = JsonString(*priority, "command.priority");
    const std::optional<core::CommandPriority> command_priority = PriorityFromString(priority_name);
    if (!command_priority.has_value()) {
      throw ParseError{"Unknown command priority: " + priority_name};
    }
    command.priority = *command_priority;
  }
  if (const JsonValue* timeout = Optional(object, "timeout_ms")) {
    command.timeout = core::CommandTimeout{JsonInt64(*timeout, "command.timeout_ms")};
  }
  if (const JsonValue* payload = Optional(object, "payload")) {
    command.payload = PayloadFromJson(*payload, "command.payload");
  }
  if (const JsonValue* metadata = Optional(object, "metadata")) {
    command.metadata = MetadataFromJson(*metadata, "command.metadata");
  }

  return command;
}

[[nodiscard]] RetryPolicy RetryPolicyFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "retry_policy");
  RetryPolicy policy;
  if (const JsonValue* max_attempts = Optional(object, "max_attempts")) {
    policy.maxAttempts = JsonUint32(*max_attempts, "retry_policy.max_attempts");
  }
  if (const JsonValue* delay = Optional(object, "delay_ms")) {
    policy.delayBetweenAttempts = RetryDelay{JsonInt64(*delay, "retry_policy.delay_ms")};
  }
  if (const JsonValue* retry_on_failure = Optional(object, "retry_on_failure")) {
    policy.retryOnFailure = JsonBool(*retry_on_failure, "retry_policy.retry_on_failure");
  }
  if (const JsonValue* retry_on_timeout = Optional(object, "retry_on_timeout")) {
    policy.retryOnTimeout = JsonBool(*retry_on_timeout, "retry_policy.retry_on_timeout");
  }
  return policy;
}

[[nodiscard]] LoopPolicy LoopPolicyFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "loop_policy");
  LoopPolicy policy;
  if (const JsonValue* iterations = Optional(object, "iterations")) {
    policy.iterations = JsonUint32(*iterations, "loop_policy.iterations");
  }
  return policy;
}

[[nodiscard]] TimeoutPolicy TimeoutPolicyFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "timeout_policy");
  TimeoutPolicy policy;
  if (const JsonValue* timeout = Optional(object, "timeout_ms")) {
    policy.timeout = TimeoutDuration{JsonInt64(*timeout, "timeout_policy.timeout_ms")};
  }
  if (const JsonValue* abort = Optional(object, "abort_on_timeout")) {
    policy.abortOnTimeout = JsonBool(*abort, "timeout_policy.abort_on_timeout");
  }
  return policy;
}

[[nodiscard]] MissionStep StepFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "step");
  MissionStep step;
  step.id = JsonUint64(Required(object, "id"), "step.id");
  step.name = JsonString(Required(object, "name"), "step.name");
  if (const JsonValue* command = Optional(object, "command")) {
    step.command = CommandFromJson(*command);
  }

  if (const JsonValue* timeout = Optional(object, "timeout_ms")) {
    step.timeout = MissionStepTimeout{JsonInt64(*timeout, "step.timeout_ms")};
  }
  if (const JsonValue* retry = Optional(object, "retry")) {
    step.retry = JsonUint32(*retry, "step.retry");
  }
  if (const JsonValue* retry_policy = Optional(object, "retry_policy")) {
    step.retryPolicy = RetryPolicyFromJson(*retry_policy);
  }
  if (const JsonValue* loop_policy = Optional(object, "loop_policy")) {
    step.loopPolicy = LoopPolicyFromJson(*loop_policy);
  }
  if (const JsonValue* timeout_policy = Optional(object, "timeout_policy")) {
    step.timeoutPolicy = TimeoutPolicyFromJson(*timeout_policy);
  }
  if (const JsonValue* wait = Optional(object, "wait")) {
    const JsonValue::Object& wait_object = AsObject(*wait, "step.wait");
    step.wait = WaitStep{
        WaitStepDuration{JsonInt64(Required(wait_object, "duration_ms"), "step.wait.duration_ms")}};
  }
  if (const JsonValue* delay = Optional(object, "delay")) {
    const JsonValue::Object& delay_object = AsObject(*delay, "step.delay");
    step.delay = DelayStep{DelayStepDuration{
        JsonInt64(Required(delay_object, "duration_ms"), "step.delay.duration_ms")}};
  }
  if (const JsonValue* skip = Optional(object, "skip")) {
    step.skip = JsonBool(*skip, "step.skip");
  }
  if (const JsonValue* abort = Optional(object, "abort")) {
    step.abort = JsonBool(*abort, "step.abort");
  }
  if (const JsonValue* enabled = Optional(object, "enabled")) {
    step.enabled = JsonBool(*enabled, "step.enabled");
  }
  if (const JsonValue* metadata = Optional(object, "metadata")) {
    step.metadata = MetadataFromJson(*metadata, "step.metadata");
  }

  return step;
}

[[nodiscard]] Mission MissionFromJson(const JsonValue& root) {
  const JsonValue::Object& object = AsObject(root, "mission");
  const JsonValue::Object* mission_object = &object;
  if (const JsonValue* mission_root = Optional(object, "mission")) {
    mission_object = &AsObject(*mission_root, "mission");
  }

  Mission mission;
  mission.id = JsonUint64(Required(*mission_object, "id"), "id");
  mission.name = JsonString(Required(*mission_object, "name"), "name");
  mission.description = JsonString(Required(*mission_object, "description"), "description");
  mission.version = JsonString(Required(*mission_object, "version"), "version");
  mission.author = JsonString(Required(*mission_object, "author"), "author");

  if (const JsonValue* metadata = Optional(*mission_object, "metadata")) {
    mission.metadata = MetadataFromJson(*metadata, "metadata");
  }

  for (const JsonValue& step_value : AsArray(Required(*mission_object, "steps"), "steps")) {
    mission.steps.push_back(StepFromJson(step_value));
  }

  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return mission;
}

struct YamlLine final {
  std::size_t indent{0U};
  std::string content;
  std::size_t number{0U};
};

[[nodiscard]] std::string StripYamlComment(std::string_view line) {
  bool in_single_quote = false;
  bool in_double_quote = false;
  for (std::size_t index = 0U; index < line.size(); ++index) {
    const char current = line[index];
    if (current == '\'' && !in_double_quote) {
      in_single_quote = !in_single_quote;
    } else if (current == '"' && !in_single_quote) {
      in_double_quote = !in_double_quote;
    } else if (current == '#' && !in_single_quote && !in_double_quote) {
      return std::string{line.substr(0U, index)};
    }
  }
  return std::string{line};
}

[[nodiscard]] std::vector<YamlLine> YamlLines(std::string_view document) {
  std::vector<YamlLine> lines;
  std::size_t line_number = 1U;
  std::size_t start = 0U;
  while (start <= document.size()) {
    const std::size_t end = document.find('\n', start);
    std::string_view raw_line = end == std::string_view::npos ? document.substr(start)
                                                              : document.substr(start, end - start);
    if (!raw_line.empty() && raw_line.back() == '\r') {
      raw_line.remove_suffix(1U);
    }

    if (raw_line.find('\t') != std::string_view::npos) {
      throw ParseError{"YAML tabs are not supported at line " + std::to_string(line_number)};
    }

    const std::string without_comment = StripYamlComment(raw_line);
    std::size_t indent = 0U;
    while (indent < without_comment.size() && without_comment[indent] == ' ') {
      ++indent;
    }

    const std::string content = Trim(std::string_view{without_comment}.substr(indent));
    if (!content.empty()) {
      if (indent % 2U != 0U) {
        throw ParseError{"YAML indentation must use multiples of two spaces at line " +
                         std::to_string(line_number)};
      }
      lines.push_back(YamlLine{indent, content, line_number});
    }

    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1U;
    ++line_number;
  }
  return lines;
}

[[nodiscard]] std::pair<std::string, std::string> SplitYamlKeyValue(const YamlLine& line) {
  const std::size_t separator = line.content.find(':');
  if (separator == std::string::npos) {
    throw ParseError{"Expected key/value mapping at YAML line " + std::to_string(line.number)};
  }
  return {Trim(std::string_view{line.content}.substr(0U, separator)),
          Trim(std::string_view{line.content}.substr(separator + 1U))};
}

[[nodiscard]] std::string YamlScalar(std::string_view value) {
  std::string trimmed = Trim(value);
  if (trimmed.size() >= 2U && ((trimmed.front() == '"' && trimmed.back() == '"') ||
                               (trimmed.front() == '\'' && trimmed.back() == '\''))) {
    trimmed = trimmed.substr(1U, trimmed.size() - 2U);
  }
  return trimmed;
}

[[nodiscard]] bool YamlBool(std::string_view value, std::string_view field_name) {
  const std::string scalar = YamlScalar(value);
  if (scalar == "true") {
    return true;
  }
  if (scalar == "false") {
    return false;
  }
  throw ParseError{std::string{field_name} + " must be true or false"};
}

[[nodiscard]] std::uint32_t ParseUint32(std::string_view text, std::string_view field_name) {
  const std::uint64_t value = ParseUint64(text, field_name);
  if (value > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
    throw ParseError{std::string{field_name} + " exceeds uint32 range"};
  }
  return static_cast<std::uint32_t>(value);
}

[[nodiscard]] core::CommandPayloadValue YamlPayloadValue(std::string_view value) {
  const std::string scalar = YamlScalar(value);
  if (scalar == "true") {
    return true;
  }
  if (scalar == "false") {
    return false;
  }

  try {
    const std::int64_t integer = ParseInt64(scalar, "payload value");
    return integer;
  } catch (const ParseError&) {
  }

  try {
    return ParseDouble(scalar, "payload value");
  } catch (const ParseError&) {
  }

  return scalar;
}

[[nodiscard]] std::size_t LogicalIndent(const YamlLine& line, std::size_t base_indent) {
  if (line.indent < base_indent) {
    throw ParseError{"YAML content escaped mission root at line " + std::to_string(line.number)};
  }
  return line.indent - base_indent;
}

void ParseYamlStringMap(const std::vector<YamlLine>& lines, std::size_t& index,
                        std::size_t expected_indent, std::size_t base_indent,
                        MissionMetadata& output) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML indentation at line " +
                       std::to_string(lines[index].number)};
    }
    if (lines[index].content.rfind("- ", 0U) == 0U) {
      return;
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (value.empty()) {
      throw ParseError{"Metadata value must be scalar at YAML line " +
                       std::to_string(lines[index].number)};
    }
    output.emplace(key, YamlScalar(value));
    ++index;
  }
}

void ParseYamlPayload(const std::vector<YamlLine>& lines, std::size_t& index,
                      std::size_t expected_indent, std::size_t base_indent,
                      core::CommandPayload& output) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (value.empty()) {
      throw ParseError{"Payload value must be scalar at YAML line " +
                       std::to_string(lines[index].number)};
    }
    output.emplace(key, YamlPayloadValue(value));
    ++index;
  }
}

void ApplyYamlCommandField(core::Command& command, std::string_view key, std::string_view value,
                           std::size_t line_number) {
  if (key == "id") {
    command.id = ParseUint64(value, "command.id");
  } else if (key == "type") {
    const std::string type = YamlScalar(value);
    const std::optional<core::CommandType> command_type = CommandTypeFromString(type);
    if (!command_type.has_value()) {
      throw ParseError{"Unknown command type at YAML line " + std::to_string(line_number) + ": " +
                       type};
    }
    command.type = *command_type;
  } else if (key == "priority") {
    const std::string priority = YamlScalar(value);
    const std::optional<core::CommandPriority> command_priority = PriorityFromString(priority);
    if (!command_priority.has_value()) {
      throw ParseError{"Unknown command priority at YAML line " + std::to_string(line_number) +
                       ": " + priority};
    }
    command.priority = *command_priority;
  } else if (key == "timeout_ms") {
    command.timeout = core::CommandTimeout{ParseInt64(value, "command.timeout_ms")};
  } else {
    throw ParseError{"Unknown command field at YAML line " + std::to_string(line_number) + ": " +
                     std::string{key}};
  }
}

void ParseYamlCommand(const std::vector<YamlLine>& lines, std::size_t& index,
                      std::size_t expected_indent, std::size_t base_indent,
                      core::Command& command) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML command indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    ++index;
    if (key == "payload") {
      if (!value.empty()) {
        throw ParseError{"command.payload must be a nested mapping"};
      }
      ParseYamlPayload(lines, index, expected_indent + 2U, base_indent, command.payload);
    } else if (key == "metadata") {
      if (!value.empty()) {
        throw ParseError{"command.metadata must be a nested mapping"};
      }
      ParseYamlStringMap(lines, index, expected_indent + 2U, base_indent, command.metadata);
    } else {
      if (value.empty()) {
        throw ParseError{"Command field requires a scalar value at YAML line " +
                         std::to_string(lines[index - 1U].number)};
      }
      ApplyYamlCommandField(command, key, value, lines[index - 1U].number);
    }
  }
}

void ParseYamlRetryPolicy(const std::vector<YamlLine>& lines, std::size_t& index,
                          std::size_t expected_indent, std::size_t base_indent,
                          RetryPolicy& policy) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML retry_policy indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (value.empty()) {
      throw ParseError{"retry_policy field requires a scalar value at YAML line " +
                       std::to_string(lines[index].number)};
    }
    if (key == "max_attempts") {
      policy.maxAttempts = ParseUint32(value, "retry_policy.max_attempts");
    } else if (key == "delay_ms") {
      policy.delayBetweenAttempts = RetryDelay{ParseInt64(value, "retry_policy.delay_ms")};
    } else if (key == "retry_on_failure") {
      policy.retryOnFailure = YamlBool(value, "retry_policy.retry_on_failure");
    } else if (key == "retry_on_timeout") {
      policy.retryOnTimeout = YamlBool(value, "retry_policy.retry_on_timeout");
    } else {
      throw ParseError{"Unknown retry_policy field at YAML line " +
                       std::to_string(lines[index].number) + ": " + std::string{key}};
    }
    ++index;
  }
}

void ParseYamlLoopPolicy(const std::vector<YamlLine>& lines, std::size_t& index,
                         std::size_t expected_indent, std::size_t base_indent, LoopPolicy& policy) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML loop_policy indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (key != "iterations" || value.empty()) {
      throw ParseError{"loop_policy.iterations requires a scalar value at YAML line " +
                       std::to_string(lines[index].number)};
    }
    policy.iterations = ParseUint32(value, "loop_policy.iterations");
    ++index;
  }
}

void ParseYamlTimeoutPolicy(const std::vector<YamlLine>& lines, std::size_t& index,
                            std::size_t expected_indent, std::size_t base_indent,
                            TimeoutPolicy& policy) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML timeout_policy indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (value.empty()) {
      throw ParseError{"timeout_policy field requires a scalar value at YAML line " +
                       std::to_string(lines[index].number)};
    }
    if (key == "timeout_ms") {
      policy.timeout = TimeoutDuration{ParseInt64(value, "timeout_policy.timeout_ms")};
    } else if (key == "abort_on_timeout") {
      policy.abortOnTimeout = YamlBool(value, "timeout_policy.abort_on_timeout");
    } else {
      throw ParseError{"Unknown timeout_policy field at YAML line " +
                       std::to_string(lines[index].number) + ": " + std::string{key}};
    }
    ++index;
  }
}

void ParseYamlWaitStep(const std::vector<YamlLine>& lines, std::size_t& index,
                       std::size_t expected_indent, std::size_t base_indent, WaitStep& wait) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML wait indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (key != "duration_ms" || value.empty()) {
      throw ParseError{"wait.duration_ms requires a scalar value at YAML line " +
                       std::to_string(lines[index].number)};
    }
    wait.duration = WaitStepDuration{ParseInt64(value, "wait.duration_ms")};
    ++index;
  }
}

void ParseYamlDelayStep(const std::vector<YamlLine>& lines, std::size_t& index,
                        std::size_t expected_indent, std::size_t base_indent, DelayStep& delay) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < expected_indent) {
      return;
    }
    if (indent != expected_indent) {
      throw ParseError{"Unexpected YAML delay indentation at line " +
                       std::to_string(lines[index].number)};
    }
    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    if (key != "duration_ms" || value.empty()) {
      throw ParseError{"delay.duration_ms requires a scalar value at YAML line " +
                       std::to_string(lines[index].number)};
    }
    delay.duration = DelayStepDuration{ParseInt64(value, "delay.duration_ms")};
    ++index;
  }
}

void ApplyYamlStepField(MissionStep& step, std::string_view key, std::string_view value,
                        std::size_t line_number) {
  if (key == "id") {
    step.id = ParseUint64(value, "step.id");
  } else if (key == "name") {
    step.name = YamlScalar(value);
  } else if (key == "timeout_ms") {
    step.timeout = MissionStepTimeout{ParseInt64(value, "step.timeout_ms")};
  } else if (key == "retry") {
    step.retry = ParseUint32(value, "step.retry");
  } else if (key == "enabled") {
    step.enabled = YamlBool(value, "step.enabled");
  } else if (key == "skip") {
    step.skip = YamlBool(value, "step.skip");
  } else if (key == "abort") {
    step.abort = YamlBool(value, "step.abort");
  } else {
    throw ParseError{"Unknown step field at YAML line " + std::to_string(line_number) + ": " +
                     std::string{key}};
  }
}

[[nodiscard]] MissionStep ParseYamlStep(const std::vector<YamlLine>& lines, std::size_t& index,
                                        std::size_t base_indent) {
  MissionStep step;
  YamlLine first_line = lines[index];
  std::string first_content = first_line.content.substr(2U);
  ++index;
  if (!Trim(first_content).empty()) {
    const auto [key, value] =
        SplitYamlKeyValue(YamlLine{first_line.indent, Trim(first_content), first_line.number});
    if (value.empty()) {
      throw ParseError{"YAML step sequence entry requires a scalar first field"};
    }
    ApplyYamlStepField(step, key, value, first_line.number);
  }

  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < 4U || (indent == 2U && lines[index].content.rfind("- ", 0U) == 0U)) {
      return step;
    }
    if (indent != 4U) {
      throw ParseError{"Unexpected YAML step indentation at line " +
                       std::to_string(lines[index].number)};
    }

    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    ++index;
    if (key == "command") {
      if (!value.empty()) {
        throw ParseError{"step.command must be a nested mapping"};
      }
      ParseYamlCommand(lines, index, 6U, base_indent, step.command);
    } else if (key == "retry_policy") {
      if (!value.empty()) {
        throw ParseError{"step.retry_policy must be a nested mapping"};
      }
      ParseYamlRetryPolicy(lines, index, 6U, base_indent, step.retryPolicy);
    } else if (key == "loop_policy") {
      if (!value.empty()) {
        throw ParseError{"step.loop_policy must be a nested mapping"};
      }
      ParseYamlLoopPolicy(lines, index, 6U, base_indent, step.loopPolicy);
    } else if (key == "timeout_policy") {
      if (!value.empty()) {
        throw ParseError{"step.timeout_policy must be a nested mapping"};
      }
      ParseYamlTimeoutPolicy(lines, index, 6U, base_indent, step.timeoutPolicy);
    } else if (key == "wait") {
      if (!value.empty()) {
        throw ParseError{"step.wait must be a nested mapping"};
      }
      WaitStep wait;
      ParseYamlWaitStep(lines, index, 6U, base_indent, wait);
      step.wait = wait;
    } else if (key == "delay") {
      if (!value.empty()) {
        throw ParseError{"step.delay must be a nested mapping"};
      }
      DelayStep delay;
      ParseYamlDelayStep(lines, index, 6U, base_indent, delay);
      step.delay = delay;
    } else if (key == "metadata") {
      if (!value.empty()) {
        throw ParseError{"step.metadata must be a nested mapping"};
      }
      ParseYamlStringMap(lines, index, 6U, base_indent, step.metadata);
    } else {
      if (value.empty()) {
        throw ParseError{"Step field requires a scalar value at YAML line " +
                         std::to_string(lines[index - 1U].number)};
      }
      ApplyYamlStepField(step, key, value, lines[index - 1U].number);
    }
  }

  return step;
}

void ParseYamlSteps(const std::vector<YamlLine>& lines, std::size_t& index, std::size_t base_indent,
                    Mission& mission) {
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent < 2U) {
      return;
    }
    if (indent != 2U || lines[index].content.rfind("- ", 0U) != 0U) {
      throw ParseError{"Expected YAML step sequence entry at line " +
                       std::to_string(lines[index].number)};
    }
    mission.steps.push_back(ParseYamlStep(lines, index, base_indent));
  }
}

void ApplyYamlMissionField(Mission& mission, std::string_view key, std::string_view value,
                           std::size_t line_number) {
  if (key == "id") {
    mission.id = ParseUint64(value, "id");
  } else if (key == "name") {
    mission.name = YamlScalar(value);
  } else if (key == "description") {
    mission.description = YamlScalar(value);
  } else if (key == "version") {
    mission.version = YamlScalar(value);
  } else if (key == "author") {
    mission.author = YamlScalar(value);
  } else {
    throw ParseError{"Unknown mission field at YAML line " + std::to_string(line_number) + ": " +
                     std::string{key}};
  }
}

[[nodiscard]] Mission MissionFromYaml(std::string_view document) {
  const std::vector<YamlLine> lines = YamlLines(document);
  if (lines.empty()) {
    throw ParseError{"YAML mission document is empty"};
  }

  std::size_t index = 0U;
  std::size_t base_indent = 0U;
  if (lines.front().indent == 0U && lines.front().content == "mission:") {
    base_indent = 2U;
    index = 1U;
  }

  Mission mission;
  while (index < lines.size()) {
    const std::size_t indent = LogicalIndent(lines[index], base_indent);
    if (indent != 0U) {
      throw ParseError{"Expected top-level mission field at YAML line " +
                       std::to_string(lines[index].number)};
    }

    const auto [key, value] = SplitYamlKeyValue(lines[index]);
    ++index;
    if (key == "metadata") {
      if (!value.empty()) {
        throw ParseError{"mission.metadata must be a nested mapping"};
      }
      ParseYamlStringMap(lines, index, 2U, base_indent, mission.metadata);
    } else if (key == "steps") {
      if (!value.empty()) {
        throw ParseError{"mission.steps must be a nested sequence"};
      }
      ParseYamlSteps(lines, index, base_indent, mission);
    } else {
      if (value.empty()) {
        throw ParseError{"Mission field requires a scalar value at YAML line " +
                         std::to_string(lines[index - 1U].number)};
      }
      ApplyYamlMissionField(mission, key, value, lines[index - 1U].number);
    }
  }

  mission.timestamp =
      std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  return mission;
}

} // namespace

MissionParseResult MissionParser::ParseJson(std::string_view document) const {
  try {
    return Success(MissionFromJson(JsonParser{document}.Parse()));
  } catch (const std::exception& exception) {
    return Failure(std::string{"JSON mission parse failed: "} + exception.what());
  } catch (...) {
    return Failure("JSON mission parse failed with an unknown error");
  }
}

MissionParseResult MissionParser::ParseYaml(std::string_view document) const {
  try {
    return Success(MissionFromYaml(document));
  } catch (const std::exception& exception) {
    return Failure(std::string{"YAML mission parse failed: "} + exception.what());
  } catch (...) {
    return Failure("YAML mission parse failed with an unknown error");
  }
}

} // namespace humanoid::mission
