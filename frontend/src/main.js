// Variable de entorno de la dirección del Weboscket
const WS_URL = import.meta.env.VITE_WS_URL
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL

export const contenedorTerminal = document.getElementById('mensajes-terminal');

export let ws;

export async function conectarWebsocket(id_equipo) {
    ws = new WebSocket(WS_URL + `${id_equipo}`);

    ws.addEventListener('close', () => {
        agregarTextoTerminal("La conexión con el dispositivo Arduino no se pudo establecer. Recargue la página");
    });    
    
    ws.addEventListener('open', () => {
        agregarTextoTerminal('La conexión con el dispositivo Arduino se estableció correctamente');
        activarMotores();
    });

}



export function agregarTextoTerminal(texto) {
    const nuevoElemento = document.createElement('p');
    const tiempo = obtenerHora();
    nuevoElemento.innerHTML = `<span>[${tiempo}]</span> ${texto}`;
    contenedorTerminal.appendChild(nuevoElemento);
    contenedorTerminal.scrollTop = contenedorTerminal.scrollHeight;
}

export function obtenerHora() {
    const fecha = new Date();
    const hora = fecha.getHours().toString().padStart(2, '0');
    const minutos = fecha.getMinutes().toString().padStart(2, '0');
    const segundos = fecha.getSeconds().toString().padStart(2, '0');
    return `${hora}:${minutos}:${segundos}`
}


export async function conectarCamara() {
    const nuevo_elemento = document.createElement('p');
    const tiempo = obtenerHora();
    
    try {   
        const pc = new RTCPeerConnection();
        pc.addEventListener('track', (evento) => {
            const webrtc_stream = document.getElementById('video-stream');
            webrtc_stream.srcObject = evento.streams[0];
        });
        
        pc.addTransceiver('video', {direction: 'recvonly'});

        const oferta = await pc.createOffer();
        await pc.setLocalDescription(oferta);

        const respuesta = await fetch(CAMARA_URL, {
            method: 'POST',
            body: oferta.sdp,
            headers: {'Content-Type': 'application/sdp'}
        });
        
        if (!respuesta.ok) {
            throw new Error('Error en el servidor WHEP');
        };

        const respuestaSdp = await respuesta.text();
        
        await pc.setRemoteDescription({type: 'answer', sdp: respuestaSdp});
        await console.log('El estado de la conexion es', pc.connectionState);

        console.log('Conexión WebRTC establecida con éxito');        
        agregarTextoTerminal('La conexión de la cámara fue establecida con éxito');

    } catch (error) {
        console.error('Fallo al conectar:', error);
        agregarTextoTerminal('La conexión de la cámara no se pudo establecer');
    }
}

function activarMotores() {
    const mensaje = {
        motores: 1,
        datos: 1
    };
    ws.send(JSON.stringify(mensaje));
}