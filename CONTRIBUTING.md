# 🌸 Contributing to SSPM

Thank you for your interest in contributing to **ShioSakura Package Manager**!

---

## Getting Started

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## What to Work On

- New backend adapters (see `src/backends/`)
- Bug fixes and test coverage
- Mirror database entries
- SPK package recipes
- Documentation improvements
- GUI frontend (SSPM Center)

## Code Style

- C++20
- Follow existing naming conventions
- All backends must implement the `Backend` abstract class
- Add tests for new functionality in `tests/`

## Submitting a PR

1. Fork the repo
2. Create a branch: `git checkout -b feature/my-feature`
3. Commit your changes
4. Push and open a Pull Request

## Adding a New Backend

1. Create `src/backends/<name>/<name>_backend.cpp`
2. Subclass `sspm::Backend`
3. Implement all pure virtual methods
4. Register in `src/resolver/resolver.cpp`
5. Add a test in `tests/backends/`

---

Questions? Open a [Discussion](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)!
