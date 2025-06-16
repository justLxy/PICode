const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');
const multer = require('multer');

const app = express();
const port = 3001;

// Setup directories
const publicDir = path.join(__dirname, 'public');
if (!fs.existsSync(publicDir)) {
    fs.mkdirSync(publicDir);
}
const uploadsDir = path.join(__dirname, 'uploads');
if (!fs.existsSync(uploadsDir)) {
    fs.mkdirSync(uploadsDir);
}

// Multer setup for file uploads
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, uploadsDir);
    },
    filename: (req, file, cb) => {
        cb(null, Date.now() + '-' + file.originalname);
    }
});
const upload = multer({ storage: storage });

app.use(cors());
app.use(express.json());
app.use('/public', express.static(publicDir));

app.post('/api/encode', upload.single('logo'), (req, res) => {
    const { message, moduleSize } = req.body;
    if (!message) {
        // Clean up uploaded file if it exists
        if(req.file) fs.unlink(req.file.path, () => {});
        return res.status(400).json({ error: 'Message is required' });
    }

    const outputFileName = `${Date.now()}.png`;
    const outputFilePath = path.join(publicDir, outputFileName);
    const encoderPath = path.join(__dirname, 'encoder');
    const logoPath = req.file ? req.file.path : '';

    const process = spawn(encoderPath, [message, logoPath, outputFilePath, '29', moduleSize || '4']);
    let isSent = false;
    let errorOutput = '';

    process.stdout.on('data', (data) => {
        if (isSent) return;
        isSent = true;
        const imageUrl = `${req.protocol}://${req.get('host')}/public/${outputFileName}`;
        res.json({ imageUrl });
    });

    process.stderr.on('data', (data) => {
        errorOutput += data.toString();
    });

    process.on('close', (code) => {
        // Clean up uploaded logo file if it exists
        if(req.file) fs.unlink(req.file.path, () => {});

        if (!isSent) {
            isSent = true;
            console.error(`Encoder stderr: ${errorOutput}`);
            if (code !== 0) {
                 res.status(500).json({ error: 'Failed to encode message.', details: errorOutput.trim() });
            }
        }
    });
});

app.post('/api/decode', upload.single('image'), (req, res) => {
    if (!req.file) {
        return res.status(400).json({ error: 'Image file is required' });
    }

    const imagePath = req.file.path;
    const decoderPath = path.join(__dirname, 'decoder');

    const process = spawn(decoderPath, [imagePath]);

    let output = '';
    let errorOutput = '';
    let isResponseSent = false;

    process.stdout.on('data', (data) => {
        output += data.toString();
    });
    
    process.stderr.on('data', (data) => {
        errorOutput += data.toString();
    });

    process.on('close', (code) => {
        if(isResponseSent) return;
        isResponseSent = true;

        fs.unlink(imagePath, () => {}); // Clean up uploaded file

        if (code === 0 && output) {
            res.json({ decodedMessage: output.trim() });
        } else {
            console.error(`Decoder stderr: ${errorOutput}`);
            res.status(500).json({ error: 'Failed to decode image.', details: errorOutput.trim() });
        }
    });
});

app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);
}); 