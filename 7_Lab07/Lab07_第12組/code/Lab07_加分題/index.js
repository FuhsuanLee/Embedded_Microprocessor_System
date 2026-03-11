const express = require("express");
const child_process = require("child_process");
const fs = require("fs");
const path = require("path");
const app = express();

app.use(express.static('./public'));

// 儲存背景程序的 process 物件
let blinkProcess = null;
let scpProcess = null;

// 主頁面路由 - 項目二：LED 控制面板
app.get("/", (req, res) => {
    res.sendFile('/public/index.html', {root: __dirname });
});

app.get("/index", (req, res) => {
    // 離開項目一/加分題，停止所有背景程序
    stopBlinkProcess();
    stopSCPProcess();
    res.sendFile('/public/index.html', {root: __dirname });
});

// 項目二：LED 控制 API
app.get("/controlLED", (req, res) => {
    console.log("LED 控制請求:", req.query);
    
    if(req.query.botton1 === "Submit") {
        const power = req.query.POWER;
        
        if(req.query.led1 === "on") {
            console.log("控制 LED1:", power);
            controlLED("LED1", power);
        }
        if(req.query.led2 === "on") {
            console.log("控制 LED2:", power);
            controlLED("LED2", power);
        }
        
        // 回傳 JSON 狀態
        res.json({
            success: true,
            led1: req.query.led1 === "on" ? power : "未選擇",
            led2: req.query.led2 === "on" ? power : "未選擇",
            timestamp: new Date().toISOString()
        });
    } else {
        res.json({ success: false, message: "無效的請求" });
    }
});

// 項目一：進入偵測模式，啟動閃爍程序
app.get("/detect", (req, res) => {
    console.log("進入偵測模式");
    
    // 啟動 LED 閃爍背景程序
    startBlinkProcess();
    
    res.sendFile('/public/detect.html', {root: __dirname });
});

// 項目一：取得光敏電阻值的 API（只讀取數值，不控制 LED）
app.get("/detecting", (req, res) => {
    console.log("讀取光敏電阻值");
    readADC()
    .then(function(data){
        res.send(data.toString().trim());
    })
    .catch(error => {
        console.error("偵測錯誤:", error);
        res.status(500).send("0");
    });
});

// 項目一：停止偵測模式
app.get("/stopDetect", (req, res) => {
    console.log("停止偵測模式");
    stopBlinkProcess();
    res.json({ success: true, message: "已停止偵測" });
});

// ===== 加分題：SCP 收容監控系統 =====

// 進入 SCP 監控模式
app.get("/scp", (req, res) => {
    console.log("進入 SCP 收容監控模式");
    
    // 啟動 SCP 監控背景程序（控制 LED 閃爍和攝影機截圖）
    startSCPProcess();
    
    res.sendFile('/public/scp.html', {root: __dirname });
});

// 取得 SCP 狀態（前端定時呼叫）
app.get("/scp_status", (req, res) => {
    readSCPStatus()
    .then(function(data){
        res.send(data.toString().trim());
    })
    .catch(error => {
        console.error("SCP 狀態讀取錯誤:", error);
        res.status(500).send("SECURE|0");
    });
});

// 停止 SCP 監控
app.get("/stopSCP", (req, res) => {
    console.log("停止 SCP 監控模式");
    stopSCPProcess();
    res.json({ success: true, message: "已停止 SCP 監控" });
});

// 啟動 SCP 背景程序
function startSCPProcess() {
    if (scpProcess) {
        stopSCPProcess();
    }
    
    console.log("啟動 SCP 監控背景程序...");
    
    scpProcess = child_process.spawn('python3', [
        "./gpio_control.py", "scp"
    ]);
    
    scpProcess.stdout.on('data', (data) => {
        // 解析輸出: PHASE:XXX|ADC:XXX|IMG:XXX
        const output = data.toString().trim();
        if (output.includes('PHASE:')) {
            console.log(`SCP 狀態: ${output}`);
        }
    });
    
    scpProcess.stderr.on('data', (data) => {
        console.error(`SCP 程序錯誤: ${data}`);
    });
    
    scpProcess.on('close', (code) => {
        console.log(`SCP 程序已結束，代碼: ${code}`);
        scpProcess = null;
    });
    
    console.log("SCP 監控背景程序已啟動，PID:", scpProcess.pid);
}

// 停止 SCP 背景程序
function stopSCPProcess() {
    if (scpProcess) {
        console.log("停止 SCP 監控背景程序，PID:", scpProcess.pid);
        scpProcess.kill('SIGTERM');
        scpProcess = null;
        
        // 確保 LED 關閉
        child_process.spawn('python3', ["./gpio_control.py", "stop"]);
    }
}

// 讀取 SCP 狀態
function readSCPStatus() {
    return new Promise(function(resolve, reject) {
        let process = child_process.spawn('python3', [
            "./gpio_control.py", "scp_status"
        ]);
        
        let output = '';
        
        process.stdout.on('data', (data) => {
            output += data.toString();
        });
        
        process.stderr.on('data', (data) => {
            console.error(`SCP 狀態錯誤: ${data}`);
        });
        
        process.on('close', (code) => {
            if (code === 0) {
                const lines = output.trim().split('\n');
                resolve(lines[lines.length - 1].trim());
            } else {
                reject("讀取失敗");
            }
        });
    });
}

// ===== 背景程序管理函數 =====

// 啟動 LED 閃爍背景程序
function startBlinkProcess() {
    // 如果已有程序在運行，先停止
    if (blinkProcess) {
        stopBlinkProcess();
    }
    
    console.log("啟動 LED 閃爍背景程序...");
    
    blinkProcess = child_process.spawn('python3', [
        "./gpio_control.py", "blink"
    ]);
    
    blinkProcess.stdout.on('data', (data) => {
        // 可選：記錄輸出
        // console.log(`閃爍程序: ${data}`);
    });
    
    blinkProcess.stderr.on('data', (data) => {
        console.error(`閃爍程序錯誤: ${data}`);
    });
    
    blinkProcess.on('close', (code) => {
        console.log(`閃爍程序已結束，代碼: ${code}`);
        blinkProcess = null;
    });
    
    console.log("LED 閃爍背景程序已啟動，PID:", blinkProcess.pid);
}

// 停止 LED 閃爍背景程序
function stopBlinkProcess() {
    if (blinkProcess) {
        console.log("停止 LED 閃爍背景程序，PID:", blinkProcess.pid);
        blinkProcess.kill('SIGTERM');
        blinkProcess = null;
        
        // 確保 LED 關閉
        child_process.spawn('python3', ["./gpio_control.py", "stop"]);
    }
}

// ===== LED 控制函數 =====

// 控制單一 LED 的函數（項目二）
function controlLED(LED, POWER){
    return new Promise(function(resolve, reject){
        // 使用統一的 gpio_control.py
        // 命令格式: python gpio_control.py LED1 on
        let process = child_process.spawn('python3',[
            "./gpio_control.py", LED, POWER
        ]);

        let output = '';
        let errorOutput = '';

        process.stdout.on('data', (data) => {
            output += data.toString();
            console.log(`LED 控制輸出: ${data}`);
        });

        process.stderr.on('data', (data) => {
            errorOutput += data.toString();
            console.error(`LED 控制錯誤: ${data}`);
        });

        process.on('close', (code) => {
            if (code === 0) {
                resolve(output);
            } else {
                reject(errorOutput || `Process exited with code ${code}`);
            }
        });
    });
}

// 只讀取光敏電阻值（不控制 LED）
function readADC(){
    return new Promise(function(resolve, reject){
        // 使用統一的 gpio_control.py
        // 命令格式: python gpio_control.py read
        let process = child_process.spawn('python3',[
            "./gpio_control.py", "read"
        ]);

        let output = '';
        let errorOutput = '';

        process.stdout.on('data', (data) => {
            output += data.toString();
            console.log(`光敏電阻值: ${data}`);
        });

        process.stderr.on('data', (data) => {
            errorOutput += data.toString();
            console.error(`讀取錯誤: ${data}`);
        });

        process.on('close', (code) => {
            if (code === 0) {
                const lines = output.trim().split('\n');
                const value = lines[lines.length - 1].trim();
                resolve(value);
            } else {
                reject(errorOutput || `Process exited with code ${code}`);
            }
        });
    });
}

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`伺服器運行於 port ${PORT}`);
});