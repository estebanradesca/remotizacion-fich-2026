const socket = new WebSocket("ws://localhost:8000/ws");
socket.onmessage = function(event) {
    const datos = event.data;
    const elemento = document.getElementById('contenedor-estado');
    elemento.innerText = datos;
} 