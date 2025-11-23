# Contributing to This Project

Thank you for your interest in contributing!
This project is an N-body simulation engine written in C++ with support for orbital mechanics, gravitational modeling, OpenGL visualization, and scientific tooling.
Contributions of all kinds are welcome — code, documentation, bug reports, ideas, examples, and more.

## 🧩 How to Contribute

### 1. Fork the Repository

Click “Fork” in the top-right corner of the GitHub page and clone your fork:
```
git clone https://github.com/eisensenpou/orbital-mechanics-engine.git
cd orbital-mechanics-engine
```
### 2. Create a Branch

Always make changes in a new branch:
```
git checkout -b feature/my-improvement
```
### 3. Make Your Changes

Follow the coding guidelines below.
Keep commits clean and meaningful.

### 4. Test Your Changes

Please ensure:

 - The project builds using CMake

 - Simulations run without crashing

 - Any new modules have minimal tests or usage examples

 - Existing behavior is not broken

### 5. Submit a Pull Request (PR)

Go to your fork and click:
```
Pull Requests → New Pull Request
```

Make sure to describe:

 - What you changed

 - Why the change is needed

 - Any known limitations

 - Screenshots or plots (if relevant)

## 🧪 Coding Guidelines

### Language & Tools

 - C++17 or newer

 - CMake for builds

 - GLAD / OpenGL for rendering

 - nlohmann/json for config parsing

### File Structure

Please keep new files consistent with the existing layout:
```
/src        → implementation
/include    → public headers
/external   → third-party libraries
/assets     → textures/data
```

### Code Style

 - Use meaningful variable and function names

 - Prefer const correctness

 - Use std::vector, std::unique_ptr, and modern C++ patterns

 - Avoid raw pointers when possible

 - Keep functions small and focused

### Documentation

 - All new classes, functions, and modules should include:

 - brief comment explaining purpose

 - parameter descriptions

 - units used (very important in physics code!)

 - references if based on standard orbital mechanics formulas

Example:
```
// Computes gravitational force between two bodies
// Params:
//   m1, m2   - masses (kg)
//   r1, r2   - positions (meters)
// Returns:
//   force vector in Newtons
```
## 🪐 Physics & Scientific Accuracy

If you contribute physics/math code:

 - ✔ Clearly state assumptions
 - ✔ Use SI units unless otherwise documented
 - ✔ Add references (papers, textbooks, NASA sources)
 - ✔ Make accuracy vs. performance trade-offs explicit
 - ✔ Avoid magic constants; define them in a header

## 🖥️ Visualization Guidelines

 - If you’re working on OpenGL code:

 - Use the GLAD loader inside /external

 - Keep rendering functions in their own modules

 - Avoid mixing simulation logic and rendering logic

 - Test on at least one of: Windows / Linux / macOS

## 🐛 Reporting Bugs

Please open an issue and include:

 - Steps to reproduce

 - OS + compiler

 - CMake version

 - Expected behavior

 - Actual behavior

 - Error logs or screenshots

## 💡 Feature Requests

Feature ideas are welcome!

Examples:

 - Improved integrators (RK4, Verlet, symplectic)

 - New visualizations

 - Stability analysis tools

 - Multi-star systems

 - JSON-based scenario loader

Open an issue and describe your idea.

## 🤝 Code of Conduct

 - Be respectful and constructive.

 - All contributions should improve the project for others.

## ⭐ Thank You for Your Interest in Contributing

Your contributions help make this simulation engine better for students, researchers, hobbyists, and developers.
If you have questions, feel free to open an issue — I’m happy to help.
