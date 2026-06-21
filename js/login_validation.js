// add event listeners for username and password in login.html
window.addEventListener("load", function () {
    const loginForm = document.getElementById("login");
    const username = document.getElementById("username");
    const password = document.getElementById("password");
    const loginMessage = document.getElementById("loginMessage");

    // give focus to username field when page loads
    username.focus();

    // validate form when user tries to submit
    loginForm.addEventListener("submit", function (event) {
        let errors = [];

        // make sure username is longer than 7 characters
        if (username.value.length < 7) {
            errors.push("Username must be at least 7 characters long.");
        }

        // make sure username is longer than 7 characters
        if (password.value.length < 7) {
            errors.push("Password must be at least 7 characters long.");
        }

        if (errors.length > 0) {
            event.preventDefault();
            loginMessage.textContent = errors.join(" ");
            loginMessage.style.color = "red";
            loginMessage.style.fontWeight = "bold";
        }
    });
});