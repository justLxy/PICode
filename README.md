
### Tech Stack

-   **Frontend**: React
-   **Backend**: Node.js / Express
-   **Core Logic**: C++
-   **Build Tools**: `make`, `g++`
-   **Package Manager**: npm

### Project Structure
PICode/
├── backend/
│   ├── cpp_src/          # C++ core encoding/decoding source code
│   │   ├── Makefile      # C++ build scripts
│   │   ├── encoder/      # Encoder source files
│   │   └── decoder/      # Decoder source files
│   ├── node_modules/
│   ├── public/           # Stores generated PIcode images
│   ├── uploads/          # Temporary uploads during decoding
│   ├── encoder           # Compiled encoder executables
│   ├── decoder           # Compiled decoder executables
│   ├── package.json
│   └── server.js         # Express back-end service
└── frontend/
    ├── public/
    ├── src/              # React front-end source code
    └── package.json

### Prerequisites

Before you begin, ensure you have the following software and libraries installed on your development machine:
1.  **Node.js and npm**: For running the backend server and frontend application.
2.  **C++ Compiler**: Such as `g++` or `clang++`, for compiling the core C++ logic.
3.  **make**: For executing the `Makefile` compilation script.
4.  **Homebrew** (macOS only): For easily installing dependencies.
5.  **Dependencies**:
    -   `libjpeg` and `libpng`: Required by the C++ code to process images.
    -   `imagemagick`: Required at runtime by the decoder for image format conversions.

You can install these dependencies with the following commands:
- **On macOS (using Homebrew):**
  ```bash
  brew install gcc libjpeg libpng imagemagick
  ```
- **On Ubuntu/Debian:**
  ```bash
  sudo apt-get update
  sudo apt-get install build-essential libjpeg-dev libpng-dev imagemagick
  ```
> **Note**: The `Makefile` includes header and library paths specific to macOS (Homebrew) (`/opt/homebrew/include`, `/opt/homebrew/lib`). If you are compiling on a different OS (like Linux), you may need to remove or modify the `CFLAGS`, `CXXFLAGS`, and `LDFLAGS` variables in the `Makefile` to match the correct paths on your system.

### Local Development

Follow these steps to run the project locally:
1.  **Compile the C++ Executables**:
    Navigate to the C++ source directory and run `make`.
    ```bash
    cd PICode/backend/cpp_src
    make
    ```
    After successful compilation, you should see `encoder` and `decoder` executables in the `PICode/backend` directory.

2.  **Install Backend Dependencies**:
    ```bash
    cd PICode/backend
    npm install
    ```

3.  **Install Frontend Dependencies**:
    ```bash
    cd PICode/frontend
    npm install
    ```

4.  **Start the Backend Server**:
    In the `PICode/backend` directory, run:
    ```bash
    npm start
    ```
    The server will start on `http://localhost:3001`.

5.  **Start the Frontend Application**:
    In the `PICode/frontend` directory, run:
    ```bash
    npm start
    ```
    The application will automatically open in your browser at `http://localhost:3000`.

You can now access the application in your browser to perform PIcode encoding and decoding.

### Deployment

Deploying this application to a production environment requires handling the frontend and backend separately.

#### Deploying the Frontend

1.  **Build Static Files**: In the `PICode/frontend` directory, run the build command.
    ```bash
    npm run build
    ```
2.  **Host Static Files**: After the build completes, all files in the `PICode/frontend/build` directory are static assets. You can deploy them to any static file hosting service, such as Vercel, Netlify, Nginx, or AWS S3.

#### Deploying the Backend

The key to deploying the backend is to ensure the server environment is consistent with the development environment, especially regarding the **C++ executables and their runtime dependencies**.

1.  **Server Environment Requirements**:
    -   **ImageMagick** must be installed.
    -   `libjpeg` and `libpng` must be installed.
    -   The C++ executables (`encoder`, `decoder`) must be compatible with the server's OS and architecture. If you compiled on macOS and your server is Linux, you **must recompile the C++ code on the Linux server**.

2.  **Deployment Steps**:
    -   Upload the `PICode/backend` directory (including `server.js`, `package.json`, and the compiled `encoder` and `decoder` files) to your server.
    -   Install Node.js and npm on the server.
    -   In the `PICode/backend` directory on the server, run `npm install --production` to install production dependencies.
    -   Use a process manager (like `pm2`) to start and manage the backend service.
      ```bash
      # Install pm2 globally
      npm install pm2 -g
      # Start the service
      pm2 start server.js --name picode-backend
      ```

#### Recommended: Deploying the Backend with Docker

To simplify backend deployment and ensure environmental consistency, using Docker is highly recommended. Below is a sample `Dockerfile` that you can place in the `PICode/backend` directory.

```Dockerfile
# Stage 1: Compile C++
FROM ubuntu:20.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libpng-dev \
    libjpeg-dev \
    make

# Copy C++ source and compile
WORKDIR /app/cpp_src
COPY ./cpp_src .
RUN make

# Stage 2: Run Node.js Application
FROM node:16

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    imagemagick \
    libjpeg-progs \
    libpng-progs

# Set working directory
WORKDIR /app

# Copy executables from the build stage
COPY --from=builder /app/encoder .
COPY --from=builder /app/decoder .

# Copy Node.js application files
COPY package*.json ./
COPY server.js ./

# Install Node.js dependencies
RUN npm install --production

# Expose port and start service
EXPOSE 3001
CMD [ "node", "server.js" ]
```

Using this `Dockerfile`, you can build an image that contains all dependencies and the compiled code, and run it on any platform that supports Docker. 
