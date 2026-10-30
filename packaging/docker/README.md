# Subsurface Docker (Browser-Based Desktop)

This Docker image runs the Subsurface desktop application in a virtual desktop environment accessible through a web browser.

It uses Xvfb to provide a virtual X11 display, Openbox as the window manager, and noVNC to make the desktop accessible through a browser. The application itself remains the standard Subsurface desktop application.

## Features

- Browser-based access to the Subsurface desktop application.
- Qt6-based application build.
- Lightweight multi-stage Docker image.
- Runs as a non-root user.
- Persistent storage for dive logs and application configuration.
- Software rendering; no GPU required.
- noVNC access through a web browser.

## Requirements

- Docker with support for multi-stage builds.
- An x86-64 (AMD64) host.
- A modern web browser.

## Building the image

Clone the Subsurface repository and build the image from the repository root:

```bash
docker build \
  -f packaging/docker/Dockerfile \
  -t subsurface-web .
```

The build compiles Subsurface from source and creates a separate runtime image containing the application and its required dependencies.

## Running the container

Create and start the container:

```bash
docker run -d \
  --name subsurface-web \
  --restart unless-stopped \
  -p 127.0.0.1:6080:6080 \
  -v subsurface-data:/data \
  -v subsurface-config:/home/subsurface \
  subsurface-web
```

This example uses named Docker volumes to persist dive data and application configuration across container restarts and replacements.

### Accessing Subsurface

Open the following URL in your browser:

```text
http://localhost:6080/vnc.html
```

The Subsurface desktop should appear in the browser.

### Storage

Two directories should be persisted:

| Container path     | Purpose                                 |
| ------------------ | --------------------------------------- |
| `/data`            | Dive log storage                        |
| `/home/subsurface` | Application configuration and user data |

The container automatically opens `/data/divelog.ssrf` when that file exists.

If using bind mounts instead of named volumes, ensure the host directories are writable by the container user (UID `10001`).

## Updating

To update the application, pull or check out the desired Subsurface source revision and rebuild the image:

```bash
docker build \
  -f packaging/docker/Dockerfile \
  -t subsurface-web .
```

Stop and remove the existing container:

```bash
docker stop subsurface-web
docker rm subsurface-web
```

Start the updated image using the same volume configuration:

```bash
docker run -d \
  --name subsurface-web \
  --restart unless-stopped \
  -p 127.0.0.1:6080:6080 \
  -v subsurface-data:/data \
  -v subsurface-config:/home/subsurface \
  subsurface-web
```

The named volumes retain their contents when the container is removed.

It is recommended to back up your dive logs before upgrading.

## Limitations

### Hardware access

The initial Docker deployment does not provide USB or Bluetooth passthrough.

Direct communication with dive computers connected to the Docker host is therefore not supported. Importing dive data through supported file-based workflows remains possible.

### Graphics and rendering

The desktop runs using Xvfb and software rendering.

No GPU is required, but rendering performance may differ from a native desktop installation.

### Browser access and security

The noVNC interface provides access to the complete Subsurface desktop session.

The VNC server does not require a password and listens only on the container's loopback interface. The browser interface is exposed on port `6080`.

The example Docker command binds the interface to `127.0.0.1` on the host, making it accessible only from the host itself.

**Do not expose port `6080` directly to the public internet.**

For remote access, use an appropriately secured VPN or an authenticated HTTPS reverse proxy.

### Architecture

The current Qt plugin configuration targets x86-64 Linux (`x86_64-linux-gnu`). Other architectures are not currently supported by this Docker configuration.

## Troubleshooting

### Cannot access the browser interface

Check that the container is running:

```bash
docker ps
```

Check the container logs:

```bash
docker logs subsurface-web
```

Confirm that port `6080` is mapped correctly.

### Application fails to start

Inspect the container logs for missing libraries, Qt plugin errors, or virtual display startup failures.

### Dive logs or settings are missing

Verify that both `/data` and `/home/subsurface` are mounted to persistent storage.

Removing a container without persistent volumes can result in data loss.
