#!/bin/bash
RED="\033[31m"
GREEN="\033[32m"
YELLOW="\033[33m"
RESET="\033[0m"

BASE_URL="http://127.0.0.1:8080"

echo "================ GET TESTS ================"

echo -e "${GREEN}\n[1] GET existing file${RESET}"
curl -v $BASE_URL/index.html
# Expected: 200 OK + HTML content

echo -e "${GREEN}\n[2] GET non-existing file${RESET}"
curl -v $BASE_URL/nope.html
# Expected: 404 Not Found

echo -e "${GREEN}\n[3] GET directory (no autoindex)${RESET}"
curl -v $BASE_URL/uploads/
# Expected: 403 Forbidden (if autoindex OFF)

echo -e "${GREEN}\n[4] GET CGI${RESET}"
curl -v $BASE_URL/test.py
# Expected: 200 OK + CGI output

echo -e "${GREEN}\n[5] GET with query string${RESET}"
curl -v "$BASE_URL/test.py?name=nouss"
# Expected: CGI receives QUERY_STRING=name=nouss

echo -e "${RED}\n================ POST TESTS ================"

echo -e "${GREEN}\n[6] POST empty body${RESET}"
curl -v -d "" $BASE_URL/test.py
# Expected: 200 OK + empty body in CGI

echo -e "${GREEN}\n[7] POST simple data${RESET}"
curl -v -d "hello=world" $BASE_URL/test.py
# Expected: CGI prints 'hello=world'

echo -e "${GREEN}\n[8] POST JSON${RESET}"
curl -v -X POST $BASE_URL/test.py \
     -H "Content-Type: application/json" \
     -d '{"name":"Nouss"}'
# Expected: CGI receives JSON string

echo -e "${GREEN}\n[9] POST to static file${RESET}"
curl -v -X POST $BASE_URL/index.html -d "test"
# Expected: 405 Method Not Allowed

echo -e "${GREEN}\n[10] POST without Content-Length (raw test)${RESET}"
printf "POST /test.py HTTP/1.1\r\nHost: localhost\r\n\r\nhello=world" | nc 127.0.0.1 8080
# Expected: 411 Length Required OR 400 Bad Request

echo -e "${GREEN}\n[11] POST wrong Content-Length${RESET}"
printf "POST /test.py HTTP/1.1\r\nHost: localhost\r\nContent-Length: 20\r\n\r\nhello" | nc 127.0.0.1 8080
# Expected: 400 Bad Request OR timeout handling

echo -e "${RED}\n================ UPLOAD TEST ================${RESET}"

echo -e "${GREEN}\n[12] Upload file${RESET}"
curl -v -F "file=@test.txt" $BASE_URL/upload
# Expected: 200 OK or 201 Created + file saved

echo -e "${RED}\n================ METHOD TESTS ================"${RESET}

echo -e "${GREEN}\n[13] DELETE method${RESET}"
curl -v -X DELETE $BASE_URL/test.py
# Expected: 405 Method Not Allowed

echo -e "${RED}\n================ ERROR TESTS ================${RESET}"

echo -e "${GREEN}\n[14] Large body (if limit configured)${RESET}"
curl -v -d "$(head -c 100000 </dev/zero | tr '\0' 'a')" $BASE_URL/test.py
# Expected: 413 Payload Too Large (if limit set)

echo -e "${GREEN}\n[15] Invalid CGI (if script broken)${RESET}"
curl -v $BASE_URL/broken.py
# Expected: 500 Internal Server Error

echo -e "${RED}\n================ DONE ================${RESET}"
# -v	montre tout (debug)
# -f	cache erreurs HTTP et fail