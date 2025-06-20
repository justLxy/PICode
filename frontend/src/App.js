import React, { useState, useRef, useEffect } from 'react';
import './App.css';

function App() {
    const [message, setMessage] = useState('');
    const [encodedImageUrl, setEncodedImageUrl] = useState('');
    const [logoFile, setLogoFile] = useState(null);
    const [selectedFile, setSelectedFile] = useState(null);
    const [decodedMessage, setDecodedMessage] = useState('');
    const [error, setError] = useState('');

    // Encoder options state
    const [moduleSize, setModuleSize] = useState('4');
    const [dimension, setDimension] = useState('29');
    const [quality, setQuality] = useState('25'); // Corresponds to λ

    // Camera related state
    const [showCamera, setShowCamera] = useState(false);
    const [stream, setStream] = useState(null);
    const [isScanning, setIsScanning] = useState(false);
    const videoRef = useRef(null);
    const canvasRef = useRef(null);
    const scanLoopId = useRef(null);
    const isScanningRef = useRef(false);
    const inflightRef = useRef(false);

    const capacityMap = {
        '29': 81,
        '33': 107,
        '37': 136,
        '41': 169,
        '45': 205,
        '49': 244,
        '53': 255,
        '57': 255,
        '61': 255,
        '65': 255,
    };

    const MAX_MESSAGE_LENGTH = capacityMap[dimension] || 81;

    useEffect(() => {
        if (message.length > MAX_MESSAGE_LENGTH) {
            setMessage(message.substring(0, MAX_MESSAGE_LENGTH));
        }
    }, [dimension, message, MAX_MESSAGE_LENGTH]);


    const handleEncode = async () => {
        setError('');
        setEncodedImageUrl('');
        
        const formData = new FormData();
        formData.append('message', message);
        formData.append('moduleSize', moduleSize);
        formData.append('dimension', dimension);
        formData.append('quality', quality);

        if (logoFile) {
            formData.append('logo', logoFile);
        }

        try {
            const response = await fetch('http://localhost:3001/api/encode', {
                method: 'POST',
                body: formData,
            });
            const data = await response.json();
            if (response.ok) {
                setEncodedImageUrl(data.imageUrl);
            } else {
                setError(data.error || 'Failed to encode.');
            }
        } catch (err) {
            setError('An error occurred during encoding.');
        }
    };

    const startCamera = async () => {
        if (scanLoopId.current) {
            cancelAnimationFrame(scanLoopId.current);
            scanLoopId.current = null;
        }
        setDecodedMessage('');
        setError('');
        setIsScanning(false);
        isScanningRef.current = false;
        inflightRef.current = false;

        try {
            const mediaStream = await navigator.mediaDevices.getUserMedia({ 
                video: { 
                    facingMode: 'environment',
                    width: { ideal: 1280 },
                    height: { ideal: 720 }
                }
            });
            setStream(mediaStream);
            setShowCamera(true);
            setError('');
            inflightRef.current = false;
            
            setTimeout(async () => {
                if (videoRef.current) {
                    videoRef.current.srcObject = mediaStream;
                    try { await videoRef.current.play(); } catch (_) {}
                    const waitUntilReady = () => {
                        if (videoRef.current && videoRef.current.videoWidth > 0 && videoRef.current.videoHeight > 0) {
                            startAutoScan();
                        } else {
                            requestAnimationFrame(waitUntilReady);
                        }
                    };
                    waitUntilReady();
                }
            }, 50);
        } catch (err) {
            console.error('Camera error:', err);
            setError('Unable to access camera. Please check permissions and try again.');
        }
    };

    const stopCamera = () => {
        if (stream) {
            stream.getTracks().forEach(track => track.stop());
            setStream(null);
        }
        if (videoRef.current) {
            videoRef.current.srcObject = null;
        }
        stopAutoScan();
        setShowCamera(false);
    };

    const startAutoScan = () => {
        setIsScanning(true);
        isScanningRef.current = true;
        scanLoopId.current = requestAnimationFrame(scanLoop);
    };

    const stopAutoScan = () => {
        setIsScanning(false);
        isScanningRef.current = false;
        if (scanLoopId.current) {
            cancelAnimationFrame(scanLoopId.current);
            scanLoopId.current = null;
        }
        inflightRef.current = false;
    };

    const scanLoop = () => {
        if (!isScanningRef.current) return;
        if (!inflightRef.current) {
            inflightRef.current = true;
            captureAndDecode().finally(() => {
                inflightRef.current = false;
                scanLoopId.current = requestAnimationFrame(scanLoop);
            });
        } else {
            scanLoopId.current = requestAnimationFrame(scanLoop);
        }
    };
    
    const captureAndDecode = () => {
        return new Promise(resolve => {
            if (!videoRef.current || !canvasRef.current || videoRef.current.paused || videoRef.current.ended) {
                return resolve();
            }

            const video = videoRef.current;
            const canvas = canvasRef.current;
            const context = canvas.getContext('2d');
            const minSide = Math.min(video.videoWidth, video.videoHeight);
            const side = minSide * 0.7;
            const sx = (video.videoWidth - side) / 2;
            const sy = (video.videoHeight - side) / 2;
            const TARGET = 600;

            canvas.width = TARGET;
            canvas.height = TARGET;

            if (TARGET === 0) return resolve();

            context.drawImage(video, sx, sy, side, side, 0, 0, TARGET, TARGET);

            canvas.toBlob(async (blob) => {
                if (blob && isScanningRef.current) {
                    const file = new File([blob], 'camera-capture.png', { type: 'image/png' });
                    const formData = new FormData();
                    formData.append('image', file);

                    try {
                        const response = await fetch('http://localhost:3001/api/decode', {
                            method: 'POST',
                            body: formData,
                        });
                        const data = await response.json();

                        if (isScanningRef.current && response.ok && data.decodedMessage && !data.decodedMessage.includes('Can not detect')) {
                            setDecodedMessage(data.decodedMessage);
                            stopCamera();
                        }
                    } catch (err) {
                        // Non-critical error, just log it
                        console.error('Decode loop error:', err);
                    }
                }
                resolve();
            }, 'image/png');
        });
    };

    const handleDecode = async (file = selectedFile) => {
        if (!file) return;
        setError('');
        setDecodedMessage('');
        const formData = new FormData();
        formData.append('image', file);

        try {
            const response = await fetch('http://localhost:3001/api/decode', {
                method: 'POST',
                body: formData,
            });
            const data = await response.json();
            if (response.ok) {
                setDecodedMessage(data.decodedMessage);
            } else {
                setError(data.error || 'Failed to decode.');
            }
        } catch (err) {
            setError('An error occurred during decoding.');
        }
    };

    const isUrl = (str) => {
        try {
            new URL(str);
            return true;
        } catch (_) {
            return false;
        }
    };

    const downloadEncodedImage = async () => {
        if (!encodedImageUrl) return;
        try {
            const response = await fetch(encodedImageUrl);
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.style.display = 'none';
            a.href = url;
            a.download = `PiCode-${Date.now()}.png`;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            a.remove();
        } catch (error) {
            console.error('Error downloading the image:', error);
            setError('Could not download the image.');
        }
    };
    

    const dimensionOptions = Object.keys(capacityMap).map(dim => (
        <option key={dim} value={dim}>{dim}x{dim}</option>
    ));

    return (
        <div className="App">
            <header>
                <h1>PiCode Generator</h1>
            </header>

            <div className="card">
                <h2>Encoder</h2>
                <div className="textarea-container">
                    <textarea
                        value={message}
                        onChange={(e) => setMessage(e.target.value)}
                        placeholder="Enter your message (e.g., a URL)"
                        maxLength={MAX_MESSAGE_LENGTH}
                    />
                    <span className="char-counter">{message.length} / {MAX_MESSAGE_LENGTH}</span>
                </div>
                
                <div className="options">
                    <div>
                        <label htmlFor="logo-upload">Logo Image:</label>
                        <input id="logo-upload" type="file" onChange={(e) => setLogoFile(e.target.files[0])} accept="image/*" />
                    </div>
                    <div>
                        <label htmlFor="dimension-select">Dimensions:</label>
                        <select id="dimension-select" value={dimension} onChange={(e) => setDimension(e.target.value)}>
                            {dimensionOptions}
                        </select>
                    </div>
                    <div>
                        <label htmlFor="quality-slider">Quality (λ): {quality}</label>
                        <input 
                            id="quality-slider"
                            type="range" 
                            min="10" 
                            max="40" 
                            value={quality} 
                            onChange={(e) => setQuality(e.target.value)}
                            className="slider"
                        />
                    </div>
                    <div>
                        <label htmlFor="module-size-select">Module Size:</label>
                        <select id="module-size-select" value={moduleSize} onChange={(e) => setModuleSize(e.target.value)}>
                            <option value="4">4x4 pixels</option>
                            <option value="5">5x5 pixels</option>
                            <option value="6">6x6 pixels</option>
                            <option value="7">7x7 pixels</option>
                            <option value="8">8x8 pixels</option>
                        </select>
                    </div>
                </div>

                <button onClick={handleEncode} disabled={!message || !logoFile}>Generate PiCode</button>

                {encodedImageUrl && (
                    <div className="result">
                        <h3>Your PiCode:</h3>
                        <img src={encodedImageUrl} alt="Encoded PiCode" />
                        <button className="download-btn" onClick={downloadEncodedImage}>Download Image</button>
                    </div>
                )}
            </div>

            <div className="card">
                <h2>Decoder</h2>
                <div className="decode-options">
                    <div className="file-upload-section">
                        <h3>Upload an Image</h3>
                        <input type="file" onChange={(e) => {
                            const file = e.target.files[0];
                            if (file) {
                                setSelectedFile(file);
                                handleDecode(file);
                            }
                        }} accept="image/*" />
                    </div>
                    <div className="camera-section">
                        <h3>Use Camera</h3>
                        {!showCamera ? (
                            <button onClick={startCamera} className="primary-button">Start Camera</button>
                        ) : (
                            <button onClick={stopCamera} className="stop-button">Stop Camera</button>
                        )}
                    </div>
                </div>
                {showCamera && (
                    <div className="camera-container">
                        <div className="video-wrapper">
                            <video ref={videoRef} className="camera-video" playsInline />
                            <div className="overlay-guide"></div>
                        </div>
                        <canvas ref={canvasRef} style={{ display: 'none' }} />
                        <div className={`scanning-indicator ${isScanning ? 'scanning' : ''}`}>
                            <p>{isScanning ? 'Scanning...' : 'Ready to scan'}</p>
                        </div>
                    </div>
                )}
                {decodedMessage && (
                    <div className="result">
                        <h3>Decoded Message:</h3>
                        {isUrl(decodedMessage) ? (
                            <a href={decodedMessage} target="_blank" rel="noopener noreferrer">{decodedMessage}</a>
                        ) : (
                            <p>{decodedMessage}</p>
                        )}
                    </div>
                )}
            </div>
             {error && <p className="error">{error}</p>}
        </div>
    );
}

export default App;
