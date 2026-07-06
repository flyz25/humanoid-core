# Validation Scripts

`hvp.py` is the Hardware Validation Program command line tool. It uses only
Python standard library modules and produces JSON and Markdown artifacts.

## Commands

```bash
python3 validation/scripts/hvp.py list-cases
python3 validation/scripts/hvp.py new-run --operator alice
python3 validation/scripts/hvp.py record --run-dir <run-dir> --case-id HVP-CONN-001 --status PASS --operator alice
python3 validation/scripts/hvp.py summarize --run-dir <run-dir>
python3 validation/scripts/hvp.py collect-log --run-dir <run-dir> --source logs
python3 validation/scripts/hvp.py add-video --run-dir <run-dir> --case-id HVP-CONN-001 --reference file:///evidence/video.mp4
```

The tool never marks a hardware test as passed by itself.
