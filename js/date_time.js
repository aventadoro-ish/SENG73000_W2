// updates the current date and time on the homepage
function updateDateTime() {

    // grab the HTML element where the date/time should be displayed
    const dateTimeElement = document.getElementById("currentDateTime");
    // error check
    if (!dateTimeElement) {
        return;
    }
    const now = new Date();

    // convert the date/time into a readable form 
    const dateTimeText = now.toLocaleString("en-CA", {
        weekday: "long",     
        year: "numeric",     
        month: "long",       
        day: "numeric",      
        hour: "numeric",     
        minute: "2-digit",  
        second: "2-digit"   
    });

    // display the formatted date/time in the HTML element
    dateTimeElement.textContent = "Current date and time: " + dateTimeText;
}


updateDateTime();

setInterval(updateDateTime, 1000);