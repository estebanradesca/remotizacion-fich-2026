import './style.css'

// Importo la variable de entorno de la dirección del Weboscket
const WS_URL = import.meta.env.VITE_WS_URL

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
        document.getElementById('caudal_valor').innerText = "No funciona";
    }
    if (datos.T) {
        document.getElementById('temp_valor').innerText = datos.T;
    } else {
        document.getElementById('temp_valor').innerText = "No funciona";
    }
    
    if (datos.R == "1") {
        document.getElementById('rele_estado').innerText = "Encendido"; 
    } else {
        document.getElementById('rele_estado').innerText = "Apagado";        
    }
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

}
/*
async function obtenerDatos2() {
    const url = 'http://localhost:8000/equipo/estado';

    const respuesta = await fetch(url, {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({id_equipo: 2, estado: "apagado"})
    });

}
*/
const info = document.getElementById('conectar');

info.onclick = obtenerDatos;


//const info2 = document.getElementById('apagar');

//info2.onclick = obtenerDatos2;