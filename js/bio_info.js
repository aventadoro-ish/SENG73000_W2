window.addEventListener("load", function () {

    // birth date
    const birthDate = new Date(2005, 11, 19); 


    const today = new Date();

    // subtract birth year from current year
    let age = today.getFullYear() - birthDate.getFullYear();

    // check if birthday has happened this year
    const birthdayThisYear = new Date(today.getFullYear(), birthDate.getMonth(), birthDate.getDate());

    if (today < birthdayThisYear) {
        age--;
    }

    //  update age element
    const ageElement = document.getElementById("age");

    if (ageElement) {
        ageElement.textContent = age;
    }

    // current year
    const currentYear = today.getFullYear();

    // update copyright element
    const copyrightElement = document.getElementById("bioCopyright");

    if (copyrightElement) {
        copyrightElement.innerHTML = "&copy; " + currentYear + " Nick Kapuka";
    }
});