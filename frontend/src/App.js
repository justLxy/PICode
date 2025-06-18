import React, { useState, useRef, useEffect } from 'react';
import './App.css';

function App() {
    const [message, setMessage] = useState('');
    const [encodedImageUrl, setEncodedImageUrl] = useState('');
    const [logoFile, setLogoFile] = useState(null);
    const [selectedFile, setSelectedFile] = useState(null);
    const [decodedMessage, setDecodedMessage] = useState('');
    const [error, setError] = useState('');

    const MAX_MESSAGE_LENGTH = 97;

    // Add state for encoder options
    const [moduleSize, setModuleSize] = useState('4');

    // Camera related state
    const [showCamera, setShowCamera] = useState(false);
    const [stream, setStream] = useState(null);
    const [isScanning, setIsScanning] = useState(false);
    const videoRef = useRef(null);
    const canvasRef = useRef(null);
    const scanLoopId = useRef(null);
    const isScanningRef = useRef(false);
    const inflightRef = useRef(false); // prevent overlapping requests

    const handleEncode = async () => {
        setError('');
        setEncodedImageUrl('');
        
        const formData = new FormData();
        formData.append('message', message);
        formData.append('moduleSize', moduleSize);
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
        // --- Hard Reset ---
        // Ensure any previous scanning loop is stopped
        if (scanLoopId.current) {
            cancelAnimationFrame(scanLoopId.current);
            scanLoopId.current = null;
        }
        // Reset all relevant state and refs to a clean slate
        setDecodedMessage('');
        setError('');
        setIsScanning(false);
        isScanningRef.current = false; // Sync ref manually
        inflightRef.current = false;
        // --- End Hard Reset ---

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
            
            // Assign stream to video element and play
            setTimeout(async () => {
                if (videoRef.current) {
                    videoRef.current.srcObject = mediaStream;
                    try { await videoRef.current.play(); } catch (_) {}
                    // Wait until metadata (dimensions) are available, then start scanning
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
            videoRef.current.srcObject = null; // Explicitly clear video source
        }
        stopAutoScan(); // Ensure scanning stops when camera closes
        setShowCamera(false);
    };

    const startAutoScan = () => {
        // No need for "if (isScanningRef.current) return;" because startCamera now handles reset
        setIsScanning(true);
        isScanningRef.current = true; // Sync ref manually
        // setError(''); // Moved to startCamera
        
        // Start the continuous scan loop
        scanLoopId.current = requestAnimationFrame(scanLoop);
    };

    const stopAutoScan = () => {
        setIsScanning(false);
        isScanningRef.current = false; // Sync ref manually
        if (scanLoopId.current) {
            cancelAnimationFrame(scanLoopId.current);
            scanLoopId.current = null; // Also clear the ref here
        }
        inflightRef.current = false;
    };

    const scanLoop = () => {
        if (!isScanningRef.current) return;
        if (!inflightRef.current) {
            inflightRef.current = true;
            captureAndDecode().finally(() => {
                inflightRef.current = false;
                // Request next frame after current decode completes
                scanLoopId.current = requestAnimationFrame(scanLoop);
            });
        } else {
            // Skip this frame, try again next frame
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

            // Crop the center 70% square of the shorter side and resize to 600x600
            const minSide = Math.min(video.videoWidth, video.videoHeight);
            const side = minSide * 0.7;
            const sx = (video.videoWidth - side) / 2;
            const sy = (video.videoHeight - side) / 2;
            const TARGET = 600;

            canvas.width = TARGET;
            canvas.height = TARGET;

            if (TARGET === 0) return resolve();

            context.drawImage(
                video,
                sx,
                sy,
                side,
                side,
                0,
                0,
                TARGET,
                TARGET
            );

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
                            setError('');
                        }
                    } catch (err) {
                        /* network or decode error; continue scanning */
                    }
                }
                resolve();
            }, 'image/png');
        });
    };

    const handleDecode = async (file = selectedFile) => {
        if (!file) {
            setError('Please select an image file or use camera to decode.');
            return;
        }
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
        if (!str) return false;
        const pattern = /^(https?:\/\/)?([\w-]+\.)+[\w-]{2,}(\/[^\s]*)?$/i;
        return pattern.test(str.trim());
    };

    useEffect(() => {
        if (decodedMessage && isUrl(decodedMessage.trim())) {
            const target = decodedMessage.trim().startsWith('http') ? decodedMessage.trim() : `https://${decodedMessage.trim()}`;
            window.open(target, '_blank');
        }
    }, [decodedMessage]);

    const downloadEncodedImage = async () => {
        if (!encodedImageUrl) return;
        try {
            const response = await fetch(encodedImageUrl);
            const blob = await response.blob();
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'picode.png';
            document.body.appendChild(a);
            a.click();
            URL.revokeObjectURL(url);
            document.body.removeChild(a);
        } catch (err) {
            console.error('Download failed', err);
            setError('Failed to download image.');
        }
    };

    return (
        <div className="App">
            <h1>PIcode Demo</h1>
            {error && <p className="error">{error}</p>}
            
            <div className="card">
                <h2>Encode</h2>
                <div className="textarea-container">
                <textarea 
                    value={message} 
                    onChange={(e) => setMessage(e.target.value)}
                    placeholder="Enter message to encode"
                        maxLength={MAX_MESSAGE_LENGTH}
                />
                    <p className="char-counter">{message.length} / {MAX_MESSAGE_LENGTH}</p>
                </div>
                <div className="options">
                    <div>
                        <label htmlFor="moduleSize">Image Size:</label>
                        <select id="moduleSize" value={moduleSize} onChange={(e) => setModuleSize(e.target.value)}>
                            <option value="4">Medium</option>
                            <option value="5">Large</option>
                            <option value="6">Extra Large</option>
                            <option value="8">Super Large</option>
                        </select>
                    </div>
                </div>
                <div>
                    <label htmlFor="logo-upload">Upload Logo:</label>
                    <input id="logo-upload" type="file" onChange={(e) => setLogoFile(e.target.files[0])} accept="image/png, image/jpeg" />
                </div>
                <button onClick={handleEncode} disabled={message.length === 0 || !logoFile}>Encode</button>
                {encodedImageUrl && (
                    <div className="result">
                        <h3>Encoded PIcode:</h3>
                        <img src={encodedImageUrl} alt="Encoded PIcode" />
                        <button onClick={downloadEncodedImage} className="download-btn">Download</button>
                    </div>
                )}
            </div>

            <div className="card">
                <h2>Decode</h2>
                <div className="decode-options">
                    <div className="file-upload-section">
                        <h3>Upload Image File</h3>
                        <input type="file" onChange={(e) => setSelectedFile(e.target.files[0])} accept="image/*" />
                        <button onClick={() => handleDecode()} disabled={!selectedFile}>Decode from File</button>
                    </div>
                    
                    <div className="camera-section">
                        <h3>Scan with Camera</h3>
                        {!showCamera ? (
                            <button onClick={startCamera}>Start Camera</button>
                        ) : (
                            <div className="camera-container">
                                <div className="video-wrapper">
                                    <video ref={videoRef} autoPlay playsInline muted className="camera-video" />
                                    <div className="overlay-guide" />
                                </div>
                                <canvas ref={canvasRef} style={{ display: 'none' }} />
                                
                                <div className={`scanning-indicator ${isScanning ? 'scanning' : ''}`}>
                                    <p>🔍 {isScanning ? 'Scanning for PIcode...' : 'Preparing camera...'}</p>
                                </div>
                                
                                <div className="camera-controls">
                                    <button onClick={stopCamera}>Close Camera</button>
                                </div>
                            </div>
                        )}
                    </div>
                </div>
                
                {decodedMessage && (
                    <div className="result">
                        <h3>Decoded Message:</h3>
                        {isUrl(decodedMessage.trim()) ? (
                            <a href={decodedMessage.trim().startsWith('http') ? decodedMessage.trim() : `https://${decodedMessage.trim()}`}
                               target="_blank" rel="noopener noreferrer">{decodedMessage}</a>
                        ) : (
                            <p>{decodedMessage}</p>
                        )}
                    </div>
                )}
            </div>
        </div>
    );
}

export default App;
