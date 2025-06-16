import React, { useState } from 'react';
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

    const handleDecode = async () => {
        if (!selectedFile) {
            setError('Please select an image file to decode.');
            return;
        }
        setError('');
        setDecodedMessage('');
        const formData = new FormData();
        formData.append('image', selectedFile);

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

    return (
        <div className="App">
            <h1>PIcode Encoder/Decoder</h1>
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
                    </div>
                )}
            </div>

            <div className="card">
                <h2>Decode</h2>
                <input type="file" onChange={(e) => setSelectedFile(e.target.files[0])} />
                <button onClick={handleDecode} disabled={!selectedFile}>Decode</button>
                {decodedMessage && (
                    <div className="result">
                        <h3>Decoded Message:</h3>
                        <p>{decodedMessage}</p>
                    </div>
                )}
            </div>
        </div>
    );
}

export default App;
