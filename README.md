# PIcode 全栈应用 (中/英)

---

## 简体中文

### 项目简介

本项目是一个功能完整的全栈 Web 应用，旨在完整复现 PIcode 的 C++ 源代码逻辑，提供 PIcode 的编码与解码功能。

### 技术栈

- **前端**: React
- **后端**: Node.js / Express
- **核心逻辑**: C++
- **构建工具**: `make`, `g++`
- **包管理器**: npm

### 项目结构

```
PICode/
├── backend/
│   ├── cpp_src/          # C++ 核心编码/解码源代码
│   │   ├── Makefile      # C++ 编译脚本
│   │   ├── encoder/      # 编码器源文件
│   │   └── decoder/      # 解码器源文件
│   ├── node_modules/
│   ├── public/           # 存放生成的 PIcode 图片
│   ├── uploads/          # 存放解码时上传的临时图片
│   ├── encoder           # (编译后生成) 编码器可执行文件
│   ├── decoder           # (编译后生成) 解码器可执行文件
│   ├── package.json
│   └── server.js         # Express 后端服务
└── frontend/
    ├── public/
    ├── src/              # React 前端源代码
    └── package.json
```

### 环境准备 (Prerequisites)

在开始之前，请确保您的开发环境中安装了以下软件和库：

1.  **Node.js 和 npm**: 用于运行后端服务和前端应用。
2.  **C++ 编译器**: 如 `g++` 或 `clang++`，用于编译 C++ 核心逻辑。
3.  **make**: 用于执行 `Makefile` 编译脚本。
4.  **Homebrew** (仅限 macOS): 用于方便地安装依赖库。
5.  **依赖库**:
    - `libjpeg` 和 `libpng`: C++ 代码依赖这两个库来处理图片。
    - `imagemagick`: 解码器在运行时需要 ImageMagick 来进行图像格式转换。

您可以通过以下命令安装这些依赖：

- **在 macOS 上 (使用 Homebrew):**
  ```bash
  brew install gcc libjpeg libpng imagemagick
  ```
- **在 Ubuntu/Debian 上:**
  ```bash
  sudo apt-get update
  sudo apt-get install build-essential libjpeg-dev libpng-dev imagemagick
  ```

> **注意**: `Makefile` 中包含了针对 macOS (Homebrew) 的头文件和库路径 (`/opt/homebrew/include`, `/opt/homebrew/lib`)。如果您在其他操作系统 (如 Linux) 上编译，可能需要移除或修改 `Makefile` 中的 `CFLAGS`, `CXXFLAGS` 和 `LDFLAGS` 变量，以匹配您系统上的正确路径。

### 本地开发 (Local Development)

请按照以下步骤在本地运行此项目：

1.  **编译 C++ 可执行文件**:
    进入 C++ 源码目录并执行 `make`。
    ```bash
    cd PICode/backend/cpp_src
    make
    ```
    编译成功后，您应该能在 `PICode/backend` 目录下看到 `encoder` 和 `decoder` 两个可执行文件。

2.  **安装后端依赖**:
    ```bash
    cd PICode/backend
    npm install
    ```

3.  **安装前端依赖**:
    ```bash
    cd PICode/frontend
    npm install
    ```

4.  **启动后端服务**:
    在 `PICode/backend` 目录下运行：
    ```bash
    npm start
    ```
    服务将启动在 `http://localhost:3001`。

5.  **启动前端应用**:
    在 `PICode/frontend` 目录下运行：
    ```bash
    npm start
    ```
    应用将在浏览器中自动打开，地址为 `http://localhost:3000`。

现在，您可以通过浏览器访问应用，进行 PIcode 的编码和解码操作。

### 部署上线 (Deployment)

将此应用部署到生产环境需要分别处理前端和后端。

#### 部署前端

1.  **构建静态文件**: 在 `PICode/frontend` 目录下运行构建命令。
    ```bash
    npm run build
    ```
2.  **托管静态文件**: 构建完成后，`PICode/frontend/build` 目录下的所有文件都是静态资源。您可以将它们部署到任何静态文件托管服务上，例如 Vercel, Netlify, Nginx 或 AWS S3。

#### 部署后端

后端部署的关键是确保服务器环境与开发环境一致，特别是 **C++ 可执行文件及其运行时依赖**。

1.  **服务器环境要求**:
    - 必须安装 **ImageMagick**。
    - 必须安装 `libjpeg` 和 `libpng`。
    - C++ 可执行文件 (`encoder`, `decoder`) 必须与服务器的操作系统和架构兼容。如果您在 macOS 上编译，而服务器是 Linux，则 **必须在 Linux 服务器上重新编译 C++ 代码**。

2.  **部署步骤**:
    - 将 `PICode/backend` 目录（包含 `server.js`, `package.json`, 以及编译好的 `encoder` 和 `decoder` 文件）上传到您的服务器。
    - 在服务器上安装 Node.js 和 npm。
    - 在服务器的 `PICode/backend` 目录下运行 `npm install --production` 来安装生产依赖。
    - 使用一个进程管理器（如 `pm2`）来启动和管理后端服务。
      ```bash
      # 全局安装 pm2
      npm install pm2 -g
      # 启动服务
      pm2 start server.js --name picode-backend
      ```

#### 推荐：使用 Docker 部署后端

为了简化后端部署并确保环境一致性，强烈建议使用 Docker。以下是一个示例 `Dockerfile`，您可以将其放在 `PICode/backend` 目录下。

```Dockerfile
# 第一阶段：编译 C++
FROM ubuntu:20.04 as builder

# 安装编译依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    libpng-dev \
    libjpeg-dev \
    make

# 复制 C++ 源码并编译
WORKDIR /app/cpp_src
COPY ./cpp_src .
RUN make

# 第二阶段：运行 Node.js 应用
FROM node:16

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    imagemagick \
    libjpeg-progs \
    libpng-progs

# 设置工作目录
WORKDIR /app

# 从编译阶段复制可执行文件
COPY --from=builder /app/encoder .
COPY --from=builder /app/decoder .

# 复制 Node.js 应用文件
COPY package*.json ./
COPY server.js ./

# 安装 Node.js 依赖
RUN npm install --production

# 暴露端口并启动服务
EXPOSE 3001
CMD [ "node", "server.js" ]
```

使用此 `Dockerfile`，您可以构建一个包含所有依赖和已编译代码的镜像，并在任何支持 Docker 的平台上运行它。

---

## English

### Introduction

This project is a full-featured, full-stack web application designed to completely replicate the logic of the original PIcode C++ source code, providing functionalities for both encoding and decoding PIcodes.

### Tech Stack

-   **Frontend**: React
-   **Backend**: Node.js / Express
-   **Core Logic**: C++
-   **Build Tools**: `make`, `g++`
-   **Package Manager**: npm

### Project Structure
(Same as above)

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