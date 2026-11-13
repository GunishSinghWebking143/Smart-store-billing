/*
  Smart Store Billing and Sales Tracker
  Core billing logic: C only.
  Browser display: HTML + CSS only (no JavaScript).
  Networking uses a small HTTP server so Chrome can open the app.

  Compile on Linux:
    gcc server.c -o smartstore
    ./smartstore

  Compile on Windows (MinGW):
    gcc server.c -o smartstore.exe -lws2_32
    smartstore.exe

  Then open: http://localhost:8080
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_type;
    #define CLOSE_SOCKET closesocket
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    typedef int socket_type;
    #define CLOSE_SOCKET close
#endif

int main(void)
{
    /* Product data stored in parallel arrays (no structures). */
    char productNames[5][20] = {"Rice", "Wheat", "Sugar", "Milk", "Cooking Oil"};
    float productPrices[5] = {60.0f, 45.0f, 50.0f, 30.0f, 150.0f};

    float totalSales = 0.0f;
    int totalCustomers = 0;
    int serverRunning = 1;

    socket_type serverSocket, clientSocket;
    struct sockaddr_in serverAddress;
#ifdef _WIN32
    int addressLength = sizeof(serverAddress);
#else
    socklen_t addressLength = sizeof(serverAddress);
#endif

#ifdef _WIN32
    WSADATA winsockData;
    if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0) {
        printf("Could not start Windows networking.\n");
        return 1;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (serverSocket == INVALID_SOCKET) {
#else
    if (serverSocket < 0) {
#endif
        printf("Could not create server socket.\n");
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Could not use port 8080. Close another server and try again.\n");
        CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        printf("Could not start listening.\n");
        CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("\nSMART STORE BILLING AND SALES TRACKER\n");
    printf("Server running at http://localhost:8080\n");
    printf("Keep this terminal open. Press Ctrl+C to stop.\n\n");

    while (serverRunning == 1) {
        char request[12000];
        char response[16000];
        char body[6000];
        char page[12000];
        int received = 0;
        int bodyLength = 0;
        int contentLength = 0;
        char *bodyStart;
        char *lengthText;
        int i;

        clientSocket = accept(serverSocket, (struct sockaddr *)&serverAddress, &addressLength);
#ifdef _WIN32
        if (clientSocket == INVALID_SOCKET) {
#else
        if (clientSocket < 0) {
#endif
            continue;
        }

        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));
        memset(body, 0, sizeof(body));
        memset(page, 0, sizeof(page));

        received = (int)recv(clientSocket, request, sizeof(request) - 1, 0);
        if (received <= 0) {
            CLOSE_SOCKET(clientSocket);
            continue;
        }

        bodyStart = strstr(request, "\r\n\r\n");
        if (bodyStart != NULL) {
            bodyStart += 4;
            lengthText = strstr(request, "Content-Length:");
            if (lengthText != NULL) {
                contentLength = atoi(lengthText + 15);
                bodyLength = received - (int)(bodyStart - request);
                while (bodyLength < contentLength && bodyLength < (int)sizeof(body) - 1) {
                    int more = (int)recv(clientSocket, request + received,
                                          (int)sizeof(request) - received - 1, 0);
                    if (more <= 0) break;
                    received += more;
                    bodyLength += more;
                }
                if (bodyLength > 0) {
                    if (bodyLength >= (int)sizeof(body)) bodyLength = (int)sizeof(body) - 1;
                    memcpy(body, bodyStart, bodyLength);
                    body[bodyLength] = '\0';
                }
            }
        }

        if (strncmp(request, "GET /style.css", 14) == 0) {
            FILE *cssFile = fopen("style.css", "rb");
            if (cssFile != NULL) {
                size_t n = fread(page, 1, sizeof(page) - 1, cssFile);
                page[n] = '\0';
                fclose(cssFile);
                snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n"
                    "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                    (unsigned long)strlen(page), page);
            } else {
                const char *missing = "style.css not found. Keep style.css beside server.c.";
                snprintf(response, sizeof(response),
                    "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n"
                    "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                    (unsigned long)strlen(missing), missing);
            }
        }
        else if (strncmp(request, "POST /bill", 10) == 0) {
            float subtotal = 0.0f;
            float discount = 0.0f;
            float finalTotal = 0.0f;
            int quantities[5] = {0, 0, 0, 0, 0};
            char fieldName[20];
            char *value;
            int anyItem = 0;

            /* Read the five quantity fields from the submitted HTML form. */
            for (i = 0; i < 5; i++) {
                sprintf(fieldName, "qty%d=", i + 1);
                value = strstr(body, fieldName);
                if (value != NULL) {
                    quantities[i] = atoi(value + strlen(fieldName));
                    if (quantities[i] < 0) quantities[i] = 0;
                    if (quantities[i] > 999) quantities[i] = 999;
                }
                if (quantities[i] > 0) {
                    subtotal = subtotal + productPrices[i] * quantities[i];
                    anyItem = 1;
                }
            }

            if (subtotal > 5000.0f) {
                discount = subtotal * 0.20f;
            } else if (subtotal > 1000.0f) {
                discount = subtotal * 0.10f;
            }

            finalTotal = subtotal - discount;
            if (anyItem == 1) {
                totalSales = totalSales + finalTotal;
                totalCustomers = totalCustomers + 1;
            }

            snprintf(page, sizeof(page),
                "<!doctype html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Smart Store - Invoice</title><link rel='stylesheet' href='/style.css'>"
                "</head><body><main class='wrap'><header class='topbar'>"
                "<div><div class='eyebrow'>SMART STORE</div><h1>Customer invoice</h1></div>"
                "<a class='link-button' href='/'>Back to billing</a></header>"
                "<section class='panel'><h2>Bill summary</h2><table><tr><th>Product</th>"
                "<th>Quantity</th><th>Rate</th><th>Amount</th></tr>");

            for (i = 0; i < 5; i++) {
                if (quantities[i] > 0) {
                    char row[300];
                    snprintf(row, sizeof(row),
                        "<tr><td>%s</td><td>%d</td><td>Rs. %.2f</td>"
                        "<td>Rs. %.2f</td></tr>",
                        productNames[i], quantities[i], productPrices[i],
                        productPrices[i] * quantities[i]);
                    strncat(page, row, sizeof(page) - strlen(page) - 1);
                }
            }

            {
                char totals[1200];
                snprintf(totals, sizeof(totals),
                    "</table><div class='totals'><p>Subtotal <strong>Rs. %.2f</strong></p>"
                    "<p>Discount <strong>Rs. %.2f</strong></p>"
                    "<p class='grand'>Final total <strong>Rs. %.2f</strong></p></div>"
                    "%s</section></main></body></html>",
                    subtotal, discount, finalTotal,
                    anyItem ? "<p class='success'>Bill added to this session's sales report.</p>"
                            : "<p class='notice'>No quantities were entered. No bill was recorded.</p>");
                strncat(page, totals, sizeof(page) - strlen(page) - 1);
            }

            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                (unsigned long)strlen(page), page);
        }
        else if (strncmp(request, "GET /report", 11) == 0) {
            snprintf(page, sizeof(page),
                "<!doctype html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Smart Store - Sales Report</title><link rel='stylesheet' href='/style.css'>"
                "</head><body><main class='wrap'><header class='topbar'>"
                "<div><div class='eyebrow'>SMART STORE</div><h1>Sales report</h1></div>"
                "<a class='link-button' href='/'>Back to dashboard</a></header>"
                "<section class='stats'><article class='stat'><span>Total sales</span>"
                "<strong>Rs. %.2f</strong></article><article class='stat'><span>"
                "Customers served</span><strong>%d</strong></article></section>"
                "<p class='notice'>This report is stored in memory while the C server is running.</p>"
                "</main></body></html>", totalSales, totalCustomers);

            snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                (unsigned long)strlen(page), page);
        }
        else {
            FILE *htmlFile = fopen("index.html", "rb");
            if (htmlFile != NULL) {
                size_t n = fread(page, 1, sizeof(page) - 1, htmlFile);
                page[n] = '\0';
                fclose(htmlFile);
                snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                    "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                    (unsigned long)strlen(page), page);
            } else {
                const char *missing = "index.html not found. Keep index.html beside server.c.";
                snprintf(response, sizeof(response),
                    "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n"
                    "Content-Length: %lu\r\nConnection: close\r\n\r\n%s",
                    (unsigned long)strlen(missing), missing);
            }
        }

        send(clientSocket, response, (int)strlen(response), 0);
        CLOSE_SOCKET(clientSocket);
    }

    CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
