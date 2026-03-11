const express = require('express');
const child_process = require("child_process");
const path = require('path');
const app = express();
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

function controlLED(LED, POWER) {
    return new Promise((resolve, reject) => {
        const process = child_process.execFile('sudo', [
            "./C++/L2Program", "LED" + LED, POWER
        ], (error, stdout, stderr) => {
            if (error) {
                reject("failed: " + error.message);
            } else {
                resolve("success");
            }
        });

        process.on('error', (err) => {
            reject("failed: " + err.message);
        });
    });
}

function ledShining(counts) {
    return new Promise((resolve, reject) => {
        const process = child_process.execFile('sudo', [
            "./C++/L2Program", "Mode_Shine", counts
        ], (error, stdout, stderr) => {
            // process 結束時呼叫
            if (error) {
                reject("failed: " + error.message);
            } else {
                resolve("success");
            }
        });

        process.on('error', (err) => {
            reject("failed: " + err.message);
        });
    });
}

// POST API
app.post('/api/led', async (req, res) => {
    try {
        const leds = req.body.leds;
        const promises = Object.entries(leds).map(([led, state]) => {
            const power = state ? "on" : "off";
            return controlLED(led, power)
                .then(result => ({ [led]: result }))
                .catch(err => ({ [led]: err }));
        });

        const resultsArray = await Promise.all(promises);
        const results = Object.assign({}, ...resultsArray);

        res.status(200).json({ results });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// POST API
app.post('/api/led/shine', async (req, res) => {
    try {
        //  { counts: 5 }
        const counts = req.body.counts;

        if (!counts || typeof counts !== 'number' || counts <= 0) {
            return res.status(400).json({ error: "Please provide a positive integer for the blink count" });
        }

        const result = await ledShining(counts);
        // const result = "success"

        res.status(200).json({ result });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});



const port = process.env.PORT || 8080;
app.listen(port, () => console.log(`Listening on port ${port}`));