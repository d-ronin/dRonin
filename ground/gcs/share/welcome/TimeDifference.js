// mostly taken from http://stackoverflow.com/questions/6108819/javascript-timestamp-to-relative-time-eg-2-seconds-ago-one-week-ago-etc-best

function relativeTime(timestamp) {

    var msPerMinute = 60 * 1000;
    var msPerHour = msPerMinute * 60;
    var msPerDay = msPerHour * 24;
    var msPerMonth = msPerDay * 30;
    var msPerYear = msPerDay * 365;

    var elapsed = Date.now() - timestamp;

    if (elapsed < msPerMinute) {
         var num = Math.round(elapsed/1000);
         return num + ' second' + (num > 1 ? 's' : '') + ' ago';   
    }

    else if (elapsed < msPerHour) {
         var num = Math.round(elapsed/msPerMinute);
         return num + ' minute' + (num > 1 ? 's' : '') + ' ago';   
    }

    else if (elapsed < msPerDay ) {
         var num = Math.round(elapsed/msPerHour);
         return num + ' hour' + (num > 1 ? 's' : '') + ' ago';   
    }

    else if (elapsed < msPerMonth) {
        var num = Math.round(elapsed/msPerDay);
        return num + ' day' + (num > 1 ? 's' : '') + ' ago';   
    }

    else if (elapsed < msPerYear) {
        var num = Math.round(elapsed/msPerMonth);
        return num + ' month' + (num > 1 ? 's' : '') + ' ago';   
    }

    else {
        var num = Math.round(elapsed/msPerYear);
        return num + ' year' + (num > 1 ? 's' : '') + ' ago';   
    }
}
