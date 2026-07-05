#include <humanoid/bt/TreeParser.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace humanoid::bt {
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

struct YamlLine final {
  std::size_t indent{0U};
  std::string text;
};

[[nodiscard]] TreeParseResult Success(TreeDocument document) {
  TreeParseResult result;
  result.document = std::move(document);
  result.success = true;
  result.message = "Behavior tree parsed successfully";
  return result;
}

[[nodiscard]] TreeParseResult Failure(std::string message) {
  TreeParseResult result;
  result.success = false;
  result.message = std::move(message);
  return result;
}

[[nodiscard]] bool IsSpace(char value) noexcept {
  return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

[[nodiscard]] std::string Trim(std::string_view text) {
  std::size_t first = 0U;
  while (first < text.size() && IsSpace(text[first])) {
    ++first;
  }

  std::size_t last = text.size();
  while (last > first && IsSpace(text[last - 1U])) {
    --last;
  }

  return std::string{text.substr(first, last - first)};
}

[[nodiscard]] bool StartsWith(std::string_view text, std::string_view prefix) noexcept {
  return text.size() >= prefix.size() && text.substr(0U, prefix.size()) == prefix;
}

[[nodiscard]] std::string UnquoteScalar(std::string value) {
  if (value.size() >= 2U && ((value.front() == '"' && value.back() == '"') ||
                             (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1U, value.size() - 2U);
  }
  return value;
}

[[nodiscard]] std::string FieldValue(std::string_view text, std::string_view field_name) {
  std::string value = UnquoteScalar(Trim(text.substr(field_name.size())));
  if (value.empty()) {
    throw ParseError{std::string{field_name.substr(0U, field_name.size() - 1U)} +
                     " must not be empty"};
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
      throw ParseError{"Unexpected trailing content in JSON behavior tree document"};
    }
    return value;
  }

private:
  [[nodiscard]] JsonValue ParseValue() {
    SkipWhitespace();
    if (position_ >= document_.size()) {
      throw ParseError{"Unexpected end of JSON behavior tree document"};
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

    throw ParseError{"Unexpected JSON behavior tree value"};
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
        throw ParseError{"JSON behavior tree object keys must be strings"};
      }
      std::string key = ParseString();
      SkipWhitespace();
      Expect(':');
      auto inserted = object.emplace(std::move(key), ParseValue());
      if (!inserted.second) {
        throw ParseError{"Duplicate JSON behavior tree object key"};
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
        throw ParseError{"JSON unicode escapes are not supported in behavior tree documents"};
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

    double value = 0.0;
    const std::string number{document_.substr(begin, position_ - begin)};
    const char* end = number.data() + number.size();
    const std::from_chars_result result = std::from_chars(number.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
      throw ParseError{"Invalid JSON number"};
    }
    return value;
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
      throw ParseError{std::string{"Expected '"} + expected + "' in JSON behavior tree document"};
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
  const auto iterator = object.find(key);
  if (iterator == object.end()) {
    throw ParseError{"Missing required field: " + std::string{key}};
  }
  return iterator->second;
}

[[nodiscard]] const JsonValue* Optional(const JsonValue::Object& object, std::string_view key) {
  const auto iterator = object.find(key);
  if (iterator == object.end()) {
    return nullptr;
  }
  return &iterator->second;
}

[[nodiscard]] std::string JsonString(const JsonValue& value, std::string_view key) {
  if (!std::holds_alternative<std::string>(value.storage)) {
    throw ParseError{std::string{key} + " must be a string"};
  }
  return std::get<std::string>(value.storage);
}

[[nodiscard]] TreeNodeDefinition NodeFromJson(const JsonValue& value, std::string_view name) {
  const JsonValue::Object& object = AsObject(value, name);
  TreeNodeDefinition node;
  node.type = JsonString(Required(object, "type"), "type");
  if (node.type.empty()) {
    throw ParseError{"Behavior tree node type must not be empty"};
  }

  const JsonValue* child = Optional(object, "child");
  const JsonValue* children = Optional(object, "children");
  if (child && children) {
    throw ParseError{"Behavior tree node cannot contain both child and children"};
  }
  if (child) {
    node.children.push_back(NodeFromJson(*child, "child"));
  }
  if (children) {
    for (const JsonValue& child_value : AsArray(*children, "children")) {
      node.children.push_back(NodeFromJson(child_value, "children[]"));
    }
  }
  return node;
}

[[nodiscard]] TreeDocument DocumentFromJson(const JsonValue& value) {
  const JsonValue::Object& object = AsObject(value, "tree document");
  TreeDocument document;
  document.root = NodeFromJson(Required(object, "root"), "root");
  return document;
}

[[nodiscard]] std::vector<YamlLine> LinesFromYaml(std::string_view document) {
  std::vector<YamlLine> lines;
  std::size_t offset = 0U;
  while (offset <= document.size()) {
    const std::size_t end = document.find('\n', offset);
    const std::string_view raw = end == std::string_view::npos
                                     ? document.substr(offset)
                                     : document.substr(offset, end - offset);

    std::size_t indent = 0U;
    while (indent < raw.size() && raw[indent] == ' ') {
      ++indent;
    }
    if (indent < raw.size() && raw[indent] == '\t') {
      throw ParseError{"YAML behavior tree indentation must use spaces"};
    }

    std::string content = Trim(raw.substr(indent));
    const std::size_t comment = content.find('#');
    if (comment != std::string::npos) {
      content = Trim(std::string_view{content}.substr(0U, comment));
    }
    if (!content.empty()) {
      lines.push_back(YamlLine{indent, std::move(content)});
    }

    if (end == std::string_view::npos) {
      break;
    }
    offset = end + 1U;
  }
  return lines;
}

void ParseYamlNodeFields(const std::vector<YamlLine>& lines, std::size_t& index, std::size_t indent,
                         TreeNodeDefinition& node);

[[nodiscard]] TreeNodeDefinition ParseYamlNodeBlock(const std::vector<YamlLine>& lines,
                                                    std::size_t& index, std::size_t indent) {
  TreeNodeDefinition node;
  ParseYamlNodeFields(lines, index, indent, node);
  return node;
}

void ParseYamlNodeList(const std::vector<YamlLine>& lines, std::size_t& index, std::size_t indent,
                       std::vector<TreeNodeDefinition>& children) {
  bool parsed_any = false;
  while (index < lines.size()) {
    const YamlLine& line = lines[index];
    if (line.indent < indent) {
      break;
    }
    if (line.indent > indent) {
      throw ParseError{"Unexpected YAML behavior tree indentation"};
    }
    if (!StartsWith(line.text, "-")) {
      break;
    }

    parsed_any = true;
    TreeNodeDefinition child;
    const std::string item = Trim(std::string_view{line.text}.substr(1U));
    ++index;
    if (!item.empty()) {
      if (!StartsWith(item, "type:")) {
        throw ParseError{"YAML behavior tree list items must start with type"};
      }
      child.type = FieldValue(item, "type:");
    }
    if (item.empty() || (index < lines.size() && lines[index].indent > indent)) {
      ParseYamlNodeFields(lines, index, indent + 2U, child);
    }
    children.push_back(std::move(child));
  }

  if (!parsed_any) {
    throw ParseError{"YAML behavior tree children list must contain at least one node"};
  }
}

void ParseYamlNodeFields(const std::vector<YamlLine>& lines, std::size_t& index, std::size_t indent,
                         TreeNodeDefinition& node) {
  bool parsed_any = false;
  while (index < lines.size()) {
    const YamlLine& line = lines[index];
    if (line.indent < indent) {
      break;
    }
    if (line.indent > indent) {
      throw ParseError{"Unexpected YAML behavior tree indentation"};
    }

    parsed_any = true;
    if (StartsWith(line.text, "-")) {
      throw ParseError{"Unexpected YAML behavior tree list item"};
    }
    if (StartsWith(line.text, "type:")) {
      if (!node.type.empty()) {
        throw ParseError{"Duplicate YAML behavior tree node type"};
      }
      node.type = FieldValue(line.text, "type:");
      ++index;
      continue;
    }
    if (line.text == "child:") {
      ++index;
      node.children.push_back(ParseYamlNodeBlock(lines, index, indent + 2U));
      continue;
    }
    if (line.text == "children:") {
      ++index;
      ParseYamlNodeList(lines, index, indent + 2U, node.children);
      continue;
    }

    throw ParseError{"Unsupported YAML behavior tree field: " + line.text};
  }

  if (!parsed_any) {
    throw ParseError{"Expected YAML behavior tree node fields"};
  }
}

[[nodiscard]] TreeDocument DocumentFromYaml(std::string_view document_text) {
  const std::vector<YamlLine> lines = LinesFromYaml(document_text);
  if (lines.empty()) {
    throw ParseError{"YAML behavior tree document is empty"};
  }
  if (lines.front().indent != 0U || lines.front().text != "root:") {
    throw ParseError{"YAML behavior tree document requires top-level root field"};
  }

  std::size_t index = 1U;
  TreeDocument document;
  document.root = ParseYamlNodeBlock(lines, index, 2U);
  if (index != lines.size()) {
    throw ParseError{"Unexpected trailing YAML behavior tree content"};
  }
  return document;
}

} // namespace

TreeParseResult TreeParser::ParseJson(std::string_view document) const {
  try {
    return Success(DocumentFromJson(JsonParser{document}.Parse()));
  } catch (const std::exception& exception) {
    return Failure(std::string{"JSON behavior tree parse failed: "} + exception.what());
  } catch (...) {
    return Failure("JSON behavior tree parse failed with an unknown error");
  }
}

TreeParseResult TreeParser::ParseYaml(std::string_view document) const {
  try {
    return Success(DocumentFromYaml(document));
  } catch (const std::exception& exception) {
    return Failure(std::string{"YAML behavior tree parse failed: "} + exception.what());
  } catch (...) {
    return Failure("YAML behavior tree parse failed with an unknown error");
  }
}

} // namespace humanoid::bt
