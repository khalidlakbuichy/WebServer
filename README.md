<p align="center">
</p>

<h1 align="center">🌐 Webserv — Moroccan C++ Webserver 🇲🇦</h1>

<p align="center">
  <strong>A lightweight HTTP/1.1 webserver written in C++98 — built as a 42 Network project.</strong>
</p>

---

## 🏗️ Overview

**Webserv** is a fully functional **HTTP webserver** implemented in **C++98**, designed to handle client requests, serve static content, and execute **CGI scripts** (PHP, Python, Ruby).  
This project demonstrates deep understanding of:

- **Socket programming**  
- **HTTP protocol**  
- **Process management & fork/exec**  
- **Configuration parsing**  
- **CGI handling**  
- **Error handling and logging**  

By building Webserv, you learn how real-world webservers like NGINX or Apache process requests under the hood.

---

## ⚙️ Features

| Feature | Description |
|:--|:--|
| 🌐 **HTTP/1.1 Support** | Handles GET, POST, and DELETE requests. |
| 📄 **Static Content Serving** | Serves HTML, CSS, JS, images, and other files. |
| 🖥️ **CGI Support** | Executes scripts in PHP, Python, and Ruby. |
| 🔄 **Multiple Clients** | Handles concurrent connections using `select()` and non-blocking sockets. |
| ⚙️ **Configuration Parsing** | Reads a custom `.conf` file to define server blocks, ports, and routes. |
| ❌ **Error Handling** | Returns proper HTTP error codes (404, 500, etc.). |
| 📋 **Logging** | Optional request and error logging. |

---

## ⚡ How Webserv Works

1. **Listen & Accept**  
   - Opens a TCP socket on the configured port.  
   - Waits for incoming client connections.  

2. **Request Parsing**  
   - Reads raw HTTP requests.  
   - Parses headers, method, path, and body.  

3. **Routing & Handling**  
   - Determines whether to serve a **static file** or execute a **CGI script**.  

4. **Response Generation**  
   - Builds an HTTP response with proper headers and body content.  
   - Sends the response back to the client.  

5. **Repeat**  
   - Continues listening for new client connections.

---

## 🧩 CGI Execution

Webserv supports dynamic content through CGI:

- **PHP**: Executes `.php` files via the system PHP interpreter.  
- **Python**: Executes `.py` scripts using Python.  
- **Ruby**: Executes `.rb` scripts using Ruby.  

CGI scripts interact with the server using **environment variables** and **stdin/stdout pipes**.

---

## 🧰 Technologies Used

- **Language:** C++98  
- **Networking:** POSIX Sockets  
- **Concurrency:** `select()` and non-blocking I/O  
- **OS:** Linux / macOS  
- **Libraries:** Standard C++ & POSIX  

---

## 🇲🇦 About the Project

**Webserv** was developed as part of the **42 Network curriculum**.  
It’s a deep dive into **low-level networking, process control, and web protocols**, giving students hands-on experience with **system-level webserver design**.  

> “Building a webserver teaches you how clients, servers, and processes communicate — the essence of the internet.”  

---

## 📜 License

This project is released under the **MIT License**.  
Feel free to explore, learn, and improve upon it.

## 🛠️ Architecture Overview

