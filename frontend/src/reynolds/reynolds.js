import '../style.css'
import './reynolds.css'

const CAMARA_URL_1 = import.meta.env.VITE_CAMARA_URL_1


// Cotrol general

const video = document.getElementById('video-stream');

const caudal = document.getElementById('valor-caudal');

const temp = document.getElementById('valor-temp');


const pasosAgua = document.getElementById('desl-pasos-agua');
pasosAgua.value = '0';

const pasosTinta = document.getElementById('desl-pasos-tinta');
pasosTinta.value = '0';

const aumPasosTinta = document.getElementById('bot-aum-pasos-tinta');
const disPasosTinta = document.getElementById('bot-dis-pasos-tinta');
const aumPasosAgua = document.getElementById('bot-aum-pasos-agua');
const disPasosAgua = document.getElementById('bot-dis-pasos-agua');

const tinta = [pasosTinta, disPasosTinta, aumPasosTinta]
const agua = [pasosAgua, disPasosAgua, aumPasosAgua]
const rele = document.getElementById('bot-rele');


import { conectarWebsocket, agregarTextoTerminal, obtenerHora, ws, 
    contenedorTerminal, conectarCamara, bloquear, desbloquear} from '../main.js';



window.addEventListener('DOMContentLoaded', () => {
    conectarCamara(CAMARA_URL_1, video);
    conectarWebsocket(0);
    agregarEscuchaWs();
});
     

pasosAgua.addEventListener('change', (evento) => {
    const nuevo_valor = evento.target.value;
    console.log(evento.constructor.name);
    const mensaje = {
        id_equipo: 0,
        pasos_agua: Number(nuevo_valor)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});


pasosTinta.addEventListener('change', (evento) => {
    const nuevo_valor = evento.target.value;

    const mensaje = {
        id_equipo: 0,
        pasos_tinta: Number(nuevo_valor)
    };
    console.log(mensaje);
    tinta.forEach((elemento) => {
        bloquear(elemento);
    });
    ws.send(JSON.stringify(mensaje));
});


disPasosTinta.addEventListener('click', (evento) => {
    if (Number(pasosTinta.value) == 0) {
        return;
    }
    pasosTinta.value = Number(pasosTinta.value) - 1;
    const mensaje = {
        id_equipo: 0,
        pasos_tinta: Number(pasosTinta.value)
    };
    console.log(mensaje);
    tinta.forEach((elemento) => {
        bloquear(elemento);
    });
    ws.send(JSON.stringify(mensaje));
});


aumPasosTinta.addEventListener('click', (evento) => {
    if (Number(pasosTinta.value) == 50) {
        return;
    }
    pasosTinta.value = Number(pasosTinta.value) + 1;
    const mensaje = {
        id_equipo: 0,
        pasos_tinta: Number(pasosTinta.value)
    };
    console.log(mensaje);
    tinta.forEach((elemento) => {
        bloquear(elemento);
    });
    ws.send(JSON.stringify(mensaje));
    
});






aumPasosAgua.addEventListener('click', (evento) => {
    if (Number(pasosAgua.value) == 600) {
        return;
    }
    pasosAgua.value = Number(pasosAgua.value) + 1;
    const mensaje = {
        id_equipo: 0,
        pasos_agua: Number(pasosAgua.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});

disPasosAgua.addEventListener('click', (evento) => {
    if (Number(pasosAgua.value) == 0) {
        return;
    }
    pasosAgua.value = Number(pasosAgua.value) - 1;
    const mensaje = {
        id_equipo: 0,
        pasos_agua: Number(pasosAgua.value)
    };
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});




rele.addEventListener('click', (evento) => {  
    let mensaje;
    if (rele.innerText == 'A') {
        rele.innerText = 'E';
        rele.classList.remove('apagado');
        rele.classList.add('encendido')
        mensaje = {
            id_equipo: 0,
            rele: 1
        };
    } else {
        rele.innerText = 'A';
        rele.classList.remove('encendido');
        rele.classList.add('apagado');
        mensaje = {
            id_equipo: 0,
            rele: 0
        };
    }
    console.log(mensaje);
    ws.send(JSON.stringify(mensaje));
});





// Botón de captura de pantalla

const botonCaptura = document.getElementById('bot-captura');

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

const botonAumentarZoom = document.getElementById('bot-aum-zoom');
const botonDisminuirZoom = document.getElementById('bot-dis-zoom');

botonAumentarZoom.addEventListener('click', aumentarZoom);
botonDisminuirZoom.addEventListener('click', disminuirZoom);

function agregarEscuchaWs() {
    ws.addEventListener('message', (evento) => {
        console.log(evento.data.trim());
        const textoProcesado = JSON.parse(evento.data.trim());
        
        console.log(textoProcesado);

        if (textoProcesado.nivel != undefined) {
            caudal.innerText = textoProcesado.nivel;
        }

        if (textoProcesado.temp != undefined) {
            temp.innerText = textoProcesado.temp;
        }     
        console.log(textoProcesado.rele);
        if (textoProcesado.rele == 1) {
            rele.innerText = 'E';
            rele.classList.remove('apagado');
            rele.classList.add('encendido');
        }    
        else if (textoProcesado.rele == 0) {
            rele.innerText = 'A'; 
            rele.classList.remove('encendido');
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

