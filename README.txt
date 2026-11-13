SMART STORE BILLING AND SALES TRACKER
=====================================

WHAT IS INCLUDED
- server.c: C program. It runs a small HTTP server and handles all billing calculations.
- index.html: Browser page structure.
- style.css: Browser appearance.
- No JavaScript, PHP or Python is used.

IMPORTANT COURSE NOTE
The billing logic uses C variables, arrays, loops, if-else and arithmetic.
The small HTTP server uses socket/networking functions. Socket programming is
not normally part of a first-semester C basics syllabus; it is only the bridge
that lets Chrome communicate with the C program. You can explain the billing
logic separately in your practical demonstration.

PRODUCTS
1. Rice - Rs. 60
2. Wheat - Rs. 45
3. Sugar - Rs. 50
4. Milk - Rs. 30
5. Cooking Oil - Rs. 150

DISCOUNTS
- Subtotal above Rs. 5,000: 20%
- Otherwise, subtotal above Rs. 1,000: 10%
- Rs. 1,000 or below: no discount

RUN ON LINUX
1. Put all three files in one folder.
2. Open a terminal in that folder.
3. Run: gcc server.c -o smartstore
4. Run: ./smartstore
5. Open Chrome and visit http://localhost:8080

RUN ON WINDOWS WITH MIN GW
1. Put all three files in one folder.
2. Open Command Prompt in that folder.
3. Compile: gcc server.c -o smartstore.exe -lws2_32
4. Run: smartstore.exe
5. Open Chrome and visit http://localhost:8080

NOTES
- Keep the terminal/server program running while using the website.
- Sales report is held in memory and resets when the C server is restarted.
- This starter version does not yet save data permanently in a database.
- The server listens on port 8080.
