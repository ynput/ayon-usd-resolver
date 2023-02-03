const express = require('express');
const app = express();

app.listen(3000, () => console.log('Listening on port 3000...'));

app.get('/api/resolve', (req, res) => {

    const op_path = String(req.headers["unresolved_path"]);
    result = {resolved_path: op_path+"____resolved" };

    res.send(result);
});
