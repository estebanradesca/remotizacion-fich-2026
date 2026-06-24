import './reynolds.css'


// Cotrol general

const video = document.getElementById('video-stream');

const caudal = document.getElementById('caudal_valor');
caudal.innerText = 'Sin datos';

const temp = document.getElementById('temp_valor');
temp.innerText = 'Sin datos';

const pasosAgua = document.getElementById('pasos_agua');
pasosAgua.value = '0';

const pasosTinta = document.getElementById('pasos_tinta');
pasosTinta.value = '0';

const masPasosTinta = document.getElementById('mas_pasos_tinta');
const menosPasosTinta = document.getElementById('menos_pasos_tinta');
const masPasosAgua = document.getElementById('mas_pasos_agua');
const menosPasosAgua = document.getElementById('menos_pasos_agua');

const rele = document.getElementById('rele-cambiar');
rele.classList.add('apagado');

// const releLuz = document.getElementById('led_rele');

// Control del streaming de la cámara

// Control del equipo
// Comunicación bidireccional mediante Websocket



import { conectarWebsocket, agregarTextoTerminal, obtenerHora, ws, 
    contenedorTerminal, conectarCamara } from '../main.js';



window.addEventListener('DOMContentLoaded', () => {
    conectarCamara();
    conectarWebsocket(0);
    agregarEscuchaWs();
});
     


pasosAgua.addEventListener('change', (evento) => {
    const nuevo_valor = evento.target.value;
    console.log(evento.constructor.name);
    const mensaje = {
        id_equipo: 1,
        pasos_agua: Number(nuevo_valor)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});


pasosTinta.addEventListener('change', (evento) => {
    const nuevo_valor = evento.target.value;

    const mensaje = {
        id_equipo: 1,
        pasos_tinta: Number(nuevo_valor)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});

masPasosAgua.addEventListener('click', (evento) => {
    if (Number(pasosAgua.value) == 600) {
        return;
    }
    pasosAgua.value = Number(pasosAgua.value) + 1;
    const mensaje = {
        id_equipo: 1,
        pasos_agua: Number(pasosAgua.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});

menosPasosAgua.addEventListener('click', (evento) => {
    if (Number(pasosAgua.value) == 0) {
        return;
    }
    pasosAgua.value = Number(pasosAgua.value) - 1;
    const mensaje = {
        id_equipo: 1,
        pasos_agua: Number(pasosAgua.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});

masPasosTinta.addEventListener('click', (evento) => {
    if (Number(pasosTinta.value) == 50) {
        return;
    }
    pasosTinta.value = Number(pasosTinta.value) + 1;
    const mensaje = {
        id_equipo: 1,
        pasos_tinta: Number(pasosTinta.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});

menosPasosTinta.addEventListener('click', (evento) => {
    if (Number(pasosTinta.value) == 0) {
        return;
    }
    pasosTinta.value = Number(pasosTinta.value) - 1;
    const mensaje = {
        id_equipo: 1,
        pasos_tinta: Number(pasosTinta.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});


rele.addEventListener('click', (evento) => {  
    let mensaje;
    if (rele.innerText == 'Apagado') {
        rele.innerText = 'Encendido';
        rele.classList.remove('apagado');
        mensaje = {
            id_equipo: 1,
            rele: 1
        };
    } else {
        rele.innerText = 'Apagado';
        rele.classList.add('apagado');
        mensaje = {
            id_equipo: 1,
            rele: 0
        };
    }
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});





// Botón de captura de pantalla

const botonCaptura = document.getElementById('boton-captura');

botonCaptura.addEventListener('click', () => {
    const captura = document.createElement('canvas');
    captura.width = 1920;
    captura.height = 1080;

    const ctx = captura.getContext('2d');

    ctx.drawImage(video, 0, 0);

    const imagenURL = captura.toDataURL('image/png');
    const enlaceDescarga = document.createElement('a');

    enlaceDescarga.href = imagenURL;
    enlaceDescarga.download = `captura_laboratorio_${Date.now()}.png`;

    enlaceDescarga.click();

});

let nivelZoom = 1;



function aumentarZoom() {
    nivelZoom += 0.5;
    if (nivelZoom > 5) {
        nivelZoom = 5; 
    } else {
        video.style.transform = `scale(${nivelZoom})`;
    }    
}

function disminuirZoom() {
    nivelZoom -= 0.5;
    if (nivelZoom < 1) {
        nivelZoom = 1;
    } else {
        video.style.transform = `scale(${nivelZoom})`;
    }    
}

const botonAumentarZoom = document.getElementById('aumentar-zoom');
const botonDisminuirZoom = document.getElementById('disminuir-zoom');

botonAumentarZoom.addEventListener('click', aumentarZoom);
botonDisminuirZoom.addEventListener('click', disminuirZoom);

function agregarEscuchaWs() {
    ws.addEventListener('message', (evento) => {
        console.log(evento.data.trim());
        const textoProcesado = JSON.parse(evento.data.trim());
        
        console.log(textoProcesado);

        if (textoProcesado.caudal_agua != undefined) {
            caudal.innerText = textoProcesado.caudal_agua;
        }

        if (textoProcesado.temp != undefined) {
            temp.innerText = textoProcesado.temp;
        }     

        if (textoProcesado.rele == 1) {
            rele.innerText = 'Encendido';
            rele.classList.remove('apagado');
        }    
        else if (textoProcesado.rele == 0) {
            rele.innerText = 'Apagado'; 
            rele.classList.add('apagado');   
    
        }

        if (textoProcesado.pasos_agua != undefined) {
            pasosAgua.value = textoProcesado.pasos_agua; 
        }
        if (textoProcesado.pasos_tinta != undefined) {
            pasosTinta.value = textoProcesado.pasos_tinta; 
        }
    });
}
