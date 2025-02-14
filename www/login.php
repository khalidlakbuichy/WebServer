<?php
ob_start();  // Start output buffering before anything else
session_start();

// Simple hardcoded credentials (in real world, use database and password hashing)
$valid_username = "admin";
$valid_password = "admin";

// Debug information
error_log("POST data: " . print_r($_POST, true));
error_log("REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD']);

// Check if form is submitted
if ($_SERVER["REQUEST_METHOD"] === "POST") {
    $username = isset($_POST['username']) ? $_POST['username'] : '';
    $password = isset($_POST['password']) ? $_POST['password'] : '';
    
    error_log("Username received: " . $username);
    error_log("Password received: " . $password);
    
    if ($username === $valid_username && $password === $valid_password) {
        $_SESSION['logged_in'] = true;
        $_SESSION['username'] = $username;
        header("Location: welcome.php");
        exit;
    } else {
        $error = "Invalid username or password";
    }
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Login</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .login-form { max-width: 300px; margin: 0 auto; }
        .form-group { margin-bottom: 15px; }
        input { width: 100%; padding: 8px; margin-top: 5px; }
        .error { color: red; margin-bottom: 10px; }
        button { background: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }
    </style>
</head>
<body>
    <div class="login-form">
        <h2>Login</h2>
        <?php if (isset($error)): ?>
            <div class="error"><?php echo htmlspecialchars($error); ?></div>
        <?php endif; ?>
        <form method="POST">
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" required>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Login</button>
        </form>
    </div>
</body>
</html>