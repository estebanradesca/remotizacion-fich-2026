import './style.css'

// Importo la variable de entorno de la dirección del Weboscket
const WS_URL = import.meta.env.VITE_WS_URL
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL
const ws = new WebSocket(WS_URL);

// Probando cosas
/* ws.addEventListener("message", (evento) => {
    console.log(typeof evento.data);
    console.log(evento.data)
});

*/

ws.addEventListener("message", (evento) => {
    const datos = JSON.parse(evento.data);
    console.log(JSON.parse(evento.data));
    console.log(typeof evento.data);
    console.log(typeof JSON.parse(evento.data));


    //document.getElementById('pasos_agua').value = datos.PA;
    //document.getElementById('pasos_tinta').value = datos.PT;
    if (datos.CA) {
        document.getElementById('caudal_valor').innerText = datos.CA;
    } else {
        document.getElementById('caudal_valor').innerText = "Sin datos";
    }
    if (datos.T) {
        document.getElementById('temp_valor').innerText = datos.T;
    } else {
        document.getElementById('temp_valor').innerText = "Sin datos";
    }
    
    if (datos.R == "1") {
        document.getElementById('rele_estado').innerText = "Encendido"; 
    } else {
        document.getElementById('rele_estado').innerText = "Apagado";        
    }
});


async function conectar() {
    try {   
        const pc = new RTCPeerConnection();
        pc.addEventListener('track', (evento) => {
            const webrtc_stream = document.getElementById('stream');
            webrtc_stream.srcObject = evento.streams[0];
        });

        pc.addTransceiver("video", { direction: "recvonly"});

        const oferta = await pc.createOffer();
        await pc.setLocalDescription(oferta);

        const respuesta = await fetch(CAMARA_URL, {
            method: "POST",
            body: oferta.sdp,
            headers: {"Content-Type": "application/sdp" }
        });

        if (!respuesta.ok) throw new Error("Error en el servidor WHEP");

        const respuestaSdp = await respuesta.text();

        await pc.setRemoteDescription({ type: "answer", sdp: respuestaSdp });
            
        console.log("Conexión WebRTC establecida con éxito");

    } catch (error) {
        console.error("Fallo al conectar:", error);
    }
}

window.addEventListener('DOMContentLoaded', () => {
    conectar();
});
     



const pasos_agua = document.getElementById('pasos_agua');
pasos_agua.addEventListener("change", (evento) => {
    const nuevo_valor = evento.target.value;
    ws.send(`ID:1|E:encendido|PA:${nuevo_valor}\n`);
});

const pasos_tinta = document.getElementById('pasos_tinta');
pasos_tinta.addEventListener("change", (evento) => {
    const nuevo_valor = evento.target.value;
    ws.send(`ID:1|E:encendido|PT:${nuevo_valor}\n`);
});




async function obtenerDatos() {
    const url = 'http://localhost:8000/equipos/estado';

    const respuesta = await fetch(url, {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({id_equipo: 1, estado: "encendido"})
    });

};

const info = document.getElementById('conectar');

info.onclick = obtenerDatos;

