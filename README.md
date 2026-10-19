# SDL3 Image Viewer

A lightweight image viewer built with **C** and **SDL3**. This project allows users to open and display images in a desktop window, with support for file selection and drag-and-drop.

## Features

- **PNG Support** — Load PNG images using `stb_image`.
- **PPM Support** — Load binary P6 PPM images.
- **Image Selection Screen** — Display a `+` icon and an "Add Image" label when the application starts without an image path.
- **File Dialog** — Select an image from your computer.
- **Drag and Drop** — Open an image by dropping it onto the application window.
- **Image Information** — Display image dimensions and first-pixel RGB values in the console when available.
- **SDL3 Rendering** — Use SDL3 to create windows and display image pixels.

## Technologies

- C
- SDL3
- stb_image
- GCC (MinGW)
- GNU Make

## Project Structure

```text
image-viewer/
├── main.c
├── stb_image.h
├── Makefile
├── README.md
└── image-viewer.exe
```

The executable is generated during compilation and does not need to be committed to Git.

## Requirements

Before building the project, install:

- GCC with MinGW
- SDL3 development libraries
- GNU Make
- Git (optional, for version control)

Ensure GCC, Make, and the SDL3 libraries are correctly configured in your system's PATH and library directories.

## Build Instructions

### 1. Clone the repository

```bash
git clone <your-repository-url>
cd image-viewer
```

Alternatively, open your existing project directory.

### 2. Build the application

```bash
make build
```

To build a Windows GUI executable without a console window, your Makefile can use:

```makefile
build:
	gcc main.c -o image_viewer.exe -lSDL3 -mwindows
```

Ensure the SDL3 library and required runtime DLLs are available to the executable.

### 3. Run the application

Start the image viewer without an argument to open the image-selection screen:

```powershell
.\image_viewer.exe
```

Open a specific image by passing its path:

```powershell
.\image_viewer.exe "spaceship.ppm"
```

For PNG:

```powershell
.\image_viewer.exe "image.png"
```

## Supported Formats

| Format | Support |
|---|---|
| PNG | Yes, through `stb_image` |
| P6 PPM | Yes, binary RGB format |
| JPEG | Not currently implemented as an advertised format |
| Other formats | Not guaranteed |

## Usage

1. Launch the application.
2. Click the `+` / **Add Image** area to open the file dialog.
3. Select a supported image.
4. Alternatively, pass an image path when launching the program or drag and drop an image onto the window.

## Implementation Notes

- PPM files must use the binary P6 format and a maximum color component value of 255.
- Image data is represented using RGB pixel data.
- `stb_image` handles PNG decoding.
- SDL3 creates the application window and renders the image.
- The GUI build uses the `-mwindows` compiler option to suppress the Windows console window.

## Future Improvements

- Add support for JPEG and additional image formats.
- Add zooming and panning.
- Add previous-image and next-image navigation.
- Add image scaling to fit the window.
- Improve the UI with a dedicated font library such as SDL_ttf.
- Add keyboard shortcuts and image metadata.


---

**Built with C and SDL3.**
