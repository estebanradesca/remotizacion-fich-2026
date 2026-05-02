const socket = new WebSocket("ws://localhost:8000/ws");
socket.onmessage = function(event) {
    const datos = event.data;
    const elemento = document.getElementById('contenedor-estado');
    elemento.innerText = datos;
} 


async function obtenerDatos() {
    const url = 'http://localhost:8000/equipo/estado';

    const respuesta = await fetch(url, {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({id_equipo: 2, estado: "funcionando"})
    });

}

async function obtenerDatos2() {
    const url = 'http://localhost:8000/equipo/estado';

    const respuesta = await fetch(url, {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({id_equipo: 2, estado: "apagado"})
    });

}

const info = document.getElementById('conectar');

info.onclick = obtenerDatos;


const info2 = document.getElementById('apagar');

info2.onclick = obtenerDatos2;