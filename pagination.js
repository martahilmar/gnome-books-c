var controls = document.getElementById("controls");
var currentPage = document.getElementById("currentpg");
var totalPages = document.getElementById("totalpg");
var slider = document.createElement("input");
var pageList;

var slide = function(){
    Book.gotoPage(slider.value);
};
var throttledSlide = _.throttle(slide, 200);
var mouseDown = false;

// Load in stored pageList from json or local storage
/*
EPUBJS.core.request("book:epub.js/examples/page_list.json").then(function(storedPageList){
    pageList = storedPageList;
    book.loadPagination(pageList);
});*/

// Or generate the pageList on the fly
Book.ready.all.then(function(){
    Book.generatePagination();
});


// Wait for the pageList to be ready and then show slider
Book.pageListReady.then(function(pageList){
    controls.style.display = "block";
    JSON.stringify(pageList); // Save the result
    slider.setAttribute("type", "range");
    slider.setAttribute("min", Book.pagination.firstPage);
    slider.setAttribute("max", Book.pagination.lastPage);
    slider.setAttribute("step", 1);
    slider.setAttribute("value", 0);

    slider.addEventListener("change", throttledSlide, false);
    slider.addEventListener("mousedown", function(){
        mouseDown = true;
    }, false);
    slider.addEventListener("mouseup", function(){
        mouseDown = false;
    }, false);

    // Wait for book to be rendered to get current page
    rendered.then(function(){
        var currentLocation = Book.getCurrentLocationCfi();
        var currentPage = Book.pagination.pageFromCfi(currentLocation);
        slider.value = currentPage;
        currentPage.value = currentPage;
    });

    controls.appendChild(slider);

    totalPages.innerText = Book.pagination.totalPages;
    currentPage.addEventListener("change", function(){
        Book.gotoPage(currentPage.value);
    }, false);
});

Book.on('book:pageChanged', function(location){
    if(!mouseDown) {
        slider.value = location.anchorPage;
    }
    currentPage.value = location.anchorPage;
});