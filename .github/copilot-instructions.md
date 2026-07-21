# Subsurface AI Coding Agent Guidelines

Welcome to the Subsurface codebase! This document provides essential guidelines for AI coding agents to be productive and effective contributors to this project. Subsurface is a dive logging application with a complex architecture and specific workflows. Follow these instructions to navigate the codebase and adhere to project conventions.

---

## Project Overview

Subsurface is a cross-platform dive logging application. It supports importing data from various dive computers, planning dives, and visualizing dive profiles. The project is structured into several major components:

- **Core Logic**: Located in `core/`, this contains the main application logic, including dive planning and data processing.
- **Desktop Widgets**: Found in `desktop-widgets/`, this handles the desktop UI components.
- **Mobile Widgets**: Found in `mobile-widgets/`, this handles the mobile UI components using QML.
- **Backend Shared**: Located in `backend-shared/`, this contains shared utilities and functions.
- **Documentation**: Found in `Documentation/`, this includes user manuals and guides.
- **Limited HTML Frontend**: Found mostly in `theme/` and export_html.cpp.

---

## Developer Workflows

### Building the Project
- Subsurface uses **CMake** as its build system.
- To build the project on Linux, run the following commands from the **parent** of the root directory:
  ```bash
  ./subsurface/scripts/build.sh -desktop -build-with-qt6
  ```
- there are many other options that -help will show you (e.g. -mobile or -build-docs)
- For macOS, run packaging/macosx/homebrew-local-build.sh
- another place to look at when it comes to building Subsurface on certain platforms is in the othe other `packaging/*` directories, as well as in the `.github/workflows`

### Running Tests
- Tests are located in the `tests/` directory.
- Use the following command to run tests:
  ```bash
  make test
  ```

---

## Project-Specific Conventions

### Coding Style
- Follow the coding style defined in `CODINGSTYLE.md`.
- Use descriptive variable names and avoid abbreviations.
- Document all functions with comments explaining their purpose.

### Patterns
- **QML Components**: Use `Kirigami` for consistent UI design in mobile widgets.
- **Data Flow**: Data is often passed between components using shared structs or JSON files.
- **Async Operations**: Use promises or callbacks for asynchronous tasks, especially in the web-related code in `theme/`.

---

## Integration Points

### External Dependencies
- Subsurface integrates with `libdivecomputer` for dive computer communication. Relevant code is in `libdivecomputer/`.
- two main data exchange formats are supported. the native XML file format (often used with .ssrf as file extension instead of .xml) and the cloud storage backend which uses git to store the data

### Cross-Component Communication
- Use shared headers in `core/` for defining interfaces between components.
- QML files in `mobile-widgets/` interact with backend logic via signals and slots.

---

## Key Files and Directories

- `core/`: Main application logic.
- `desktop-widgets/`: Desktop UI components.
- `mobile-widgets/`: Mobile UI components.
- `backend-shared/`: Shared utilities.
- `tests/`: Test cases.
- `Documentation/`: User manuals and guides.
- `theme/`: Web-related assets and scripts.

---

## Examples

### Adding a New Dive Planner Feature
1. Add core logic in `core/planner.cpp`.
2. Update the desktop UI in `desktop-widgets/`.
3. Update the mobile UI in `mobile-widgets/qml/DivePlannerSetup.qml`.

### Debugging a Mobile UI Issue
1. Check the QML files in `mobile-widgets/`.
2. Use `console.log` for debugging QML components.
3. Ensure the backend logic in `core/` is correctly linked.

---

By following these guidelines, you can contribute effectively to the Subsurface project. If you have any questions, refer to the `CONTRIBUTING.md` file or ask the maintainers.
