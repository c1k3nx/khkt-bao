# Contributing to IoT Greenhouse Project

Thank you for considering contributing to this project! 🌿

## How to Contribute

### Reporting Bugs

If you find a bug, please open an issue with:
- Clear title and description
- Steps to reproduce
- Expected vs actual behavior
- Hardware/software versions
- Logs/screenshots if applicable

### Suggesting Features

Feature suggestions are welcome! Please include:
- Use case description
- Proposed implementation (if you have ideas)
- Why this feature would be useful

### Code Contributions

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/your-feature-name`
3. **Make your changes**
4. **Test thoroughly**
5. **Commit with clear messages**: `git commit -m "Add feature X"`
6. **Push to your fork**: `git push origin feature/your-feature-name`
7. **Open a Pull Request**

## Code Standards

### Firmware (Arduino/ESP)

- Use meaningful variable names
- Add comments for complex logic
- Follow existing code style (indentation, naming)
- Test on real hardware before submitting
- Document pin changes in `docs/pinout-*.md`

### Python (PC-Vision/App)

- Follow PEP 8 style guide
- Use type hints where applicable
- Add docstrings to functions/classes
- Include unit tests for new features
- Update `requirements.txt` if adding dependencies

### Documentation

- Use clear, concise language
- Include code examples
- Update README if adding major features
- Add diagrams/screenshots if helpful

## Testing Checklist

Before submitting PR, please test:

- [ ] Code compiles without errors/warnings
- [ ] Hardware connections work as expected
- [ ] MQTT communication functional
- [ ] UART protocol intact
- [ ] Auto control logic correct
- [ ] UI/UX not broken
- [ ] No regressions in existing features

## Git Commit Guidelines

### Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Formatting, missing semicolons, etc.
- `refactor`: Code restructuring without behavior change
- `test`: Adding/updating tests
- `chore`: Maintenance tasks

### Examples

```
feat(uno): Add MQ-135 air quality sensor support

- Added analog read on A6
- Published to gh/sensor/air_quality
- Updated pinout docs

Closes #42
```

```
fix(esp8266): Fix MQTT reconnect infinite loop

Added exponential backoff to prevent rapid reconnect attempts
that overwhelm the broker.

Fixes #38
```

## Pull Request Process

1. **Update docs** if you changed behavior/API
2. **Update CHANGELOG.md** with your changes
3. **Link related issues** in PR description
4. **Request review** from maintainers
5. **Address review feedback**
6. **Squash commits** if requested
7. **Celebrate** when merged! 🎉

## Community Guidelines

- Be respectful and inclusive
- Help newcomers
- Provide constructive feedback
- Share knowledge and experience

## Questions?

Feel free to:
- Open an issue for discussion
- Comment on existing issues
- Reach out to maintainers

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

Thank you for making this project better! 🙏
