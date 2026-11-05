const previews = {
  ok: {
    code: '200 OK',
    text: `HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 4621
Connection: close

<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>CoreServer</title>
  </head>
  <body>
    <h1>Welcome to CoreServer</h1>
  </body>
</html>`
  },
  missing: {
    code: '404 Not Found',
    text: `HTTP/1.1 404 Not Found
Content-Type: text/html; charset=utf-8
Content-Length: 238
Connection: close

<html>
  <body>
    <h1>404 — Not Found</h1>
    <p>The requested resource does not exist.</p>
  </body>
</html>`
  },
  method: {
    code: '405 Method Not Allowed',
    text: `HTTP/1.1 405 Method Not Allowed
Content-Type: text/html; charset=utf-8
Content-Length: 214
Connection: close

<html>
  <body>
    <h1>405 — Method Not Allowed</h1>
    <p>This server currently supports GET and HEAD.</p>
  </body>
</html>`
  }
};

const buttons = document.querySelectorAll('.request');
const responseCode = document.getElementById('code');
const responseBody = document.getElementById('response');

buttons.forEach((button) => {
  button.addEventListener('click', () => {
    buttons.forEach((item) => item.classList.remove('active'));
    button.classList.add('active');

    const preview = previews[button.dataset.kind];
    responseCode.textContent = preview.code;
    responseBody.textContent = preview.text;
  });
});
