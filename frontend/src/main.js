import './style.css'

// Importo la variable de entorno de la dirección del Weboscket
const WS_URL = import.meta.env.VITE_WS_URL
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL


const contenedorTerminal = document.getElementById("mensajes-terminal")

const ws = new WebSocket(WS_URL);
const nuevoElemento = document.createElement("p");
const tiempo = obtenerHora();
nuevoElemento.innerHTML = `<span>[${tiempo}]</span> La conexión con el servidor se estableció correctamente.`;
contenedorTerminal.appendChild(nuevoElemento);
contenedorTerminal.scrollTop = contenedorTerminal.scrollHeight;




const caudal = document.getElementById("caudal_valor");
caudal.innerText = "Sin datos";

const temp = document.getElementById("temp_valor");
temp.innerText = "Sin datos";

const rele = document.getElementById("rele_estado");
rele.innerText = "Sin datos";

const pasosAgua = document.getElementById("pasos_agua");
pasosAgua.value = "0";

const pasosTinta = document.getElementById("pasos_tinta");
pasosTinta.value = "0";



ws.addEventListener("message", (evento) => {
    console.log(evento.data.trim());
    const datos = JSON.parse(evento.data.trim());
    
    console.log(datos);

    if (datos.caudal_agua != undefined) {
        caudal.innerText = datos.caudal_agua;
    }

    if (datos.temp != undefined) {
        temp.innerText = datos.temp;
    }     

    if (datos.rele == 1) {
        rele.innerText = "Encendido"; 
    } else if (datos.rele == 0) {
        rele.innerText = "Apagado";        
    }

    if (datos.pasos_agua != undefined) {
        pasosAgua.value = datos.pasos_agua; 
    }
    if (datos.pasos_tinta != undefined) {
        pasosTinta.value = datos.pasos_tinta; 
    }

    if (datos.mensaje != undefined) {
        const nuevo_elemento = document.createElement('p');
        const tiempo = obtenerHora();
        nuevo_elemento.innerHTML = `<span>[${tiempo}]</span> ${datos.mensaje}`;
        contenedorTerminal.appendChild(nuevo_elemento);

        contenedorTerminal.scrollTop = contenedorTerminal.scrollHeight;
    }

});

function obtenerHora() {
    const fecha = new Date();
    const hora = fecha.getHours().toString().padStart(2, '0');
    const minutos = fecha.getMinutes().toString().padStart(2, '0');
    const segundos = fecha.getSeconds().toString().padStart(2, '0');
    return `${hora}:${minutos}:${segundos}`
}

async function conectar() {
    const nuevo_elemento = document.createElement("p");
    const tiempo = obtenerHora();

    try {   
        const pc = new RTCPeerConnection();
        pc.addEventListener('track', (evento) => {
            const webrtc_stream = document.getElementById('video-stream');
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

        
        nuevo_elemento.innerHTML = `<span>[${tiempo}]</span> La conexión de la cámara fue establecida con éxito`;
        contenedorTerminal.appendChild(nuevo_elemento);
        contenedorTerminal.scrollTop = contenedorTerminal.scrollHeight;

    } catch (error) {
        console.error("Fallo al conectar:", error);
        nuevo_elemento.innerHTML = `<span>[${tiempo}]</span> La conexión de la cámara no se pudo establecer`;
        contenedorTerminal.appendChild(nuevo_elemento);
        contenedorTerminal.scrollTop = contenedorTerminal.scrollHeight;
    }
}

window.addEventListener('DOMContentLoaded', () => {
    conectar();
});
     



pasosAgua.addEventListener("change", (evento) => {
    const nuevo_valor = evento.target.value;

    const mensaje = {
        id_equipo: 1,
        pasos_agua: Number(nuevo_valor)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});


pasosTinta.addEventListener("change", (evento) => {
    const nuevo_valor = evento.target.value;

    const mensaje = {
        id_equipo: 1,
        pasos_tinta: Number(nuevo_valor)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});




async function obtenerDatos() {
    const mensaje = {
        id_equipo: 1,
        pasos_agua: 0,
        pasos_tinta: 0,
        rele: 0,
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
    rele.innerText = "Apagado";
    temp.innerText = "Sin datos";
    caudal.innerText = "Sin datos";
    pasosAgua.value = 0;
    pasosTinta.value = 0;

    
};
 

const info = document.getElementById('boton-restablecer');

info.onclick = obtenerDatos;

