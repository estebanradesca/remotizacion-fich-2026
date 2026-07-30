import '../style.css'
import './tuberias.css'

// Variable de entorno de la dirección del Weboscket
const WS_URL = import.meta.env.VITE_WS_URL
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL

// Cotrol general

import { conectarWebsocket, agregarTextoTerminal, obtenerHora, ws, 
    contenedorTerminal, conectarCamara} from '../main.js';

window.addEventListener('DOMContentLoaded', () => {
    conectarCamara();
    conectarWebsocket(1);
});


const configuracionValvulas = [
    { id: 'valvula-1', claseTramo: 'tramo-can-pvc' },
    { id: 'valvula-2', claseTramo: 'tramo-can-gal' },
    { id: 'valvula-3', claseTramo: 'tramo-can-3' },
    { id: 'valvula-4', claseTramo: 'tramo-can-4' }    
];


const botonValvula1 = document.getElementById('boton-valvula-1');
const botonValvula2 = document.getElementById('boton-valvula-2');
const botonValvula3 = document.getElementById('boton-valvula-3');
const botonValvula4 = document.getElementById('boton-valvula-4');

const botonesValvula = [botonValvula1, botonValvula2, botonValvula3, botonValvula4];

let estadoValvula = ["cerrada", "cerrada", "cerrada", "cerrada"];

function modificarValvula(numeroValvula) {
    const botonValvula = botonesValvula[numeroValvula];
    const valvula = document.getElementById(configuracionValvulas[numeroValvula].id);
    const tramo = document.getElementsByClassName(configuracionValvulas[numeroValvula].claseTramo);

    if (estadoValvula[numeroValvula] == "cerrada") {

        estadoValvula[numeroValvula] = "abierta";
        botonValvula.textContent = 'Abierta';

        botonValvula.classList.add('abierta');

        console.log(botonValvula.classList);

        valvula.classList.add('abierta');
        for (let i = 0; i < tramo.length; i++) {
            tramo[i].classList.add('activo');
        };
    } else {
        estadoValvula[numeroValvula] = "cerrada";
        botonValvula.textContent = 'Cerrada';

        botonValvula.classList.remove('abierta');
        valvula.classList.remove('abierta');

        for (let i = 0; i < tramo.length; i++) {
            tramo[i].classList.remove('activo');
        };

        const indice = estadoValvula.findIndex( estado => estado === "abierta");
        if ( indice != -1) {
            const tramoActivo = document.getElementsByClassName(configuracionValvulas[indice].claseTramo);
            for (let j = 0; j < tramoActivo.length; j++) {
                tramoActivo[j].classList.add('activo');
            };
        };
        
        
    }
}


for (let i = 0; i < 4; i++) {
    const botonValvula = document.getElementById(`boton-valvula-${i+1}`);

    botonValvula.addEventListener('click', () => {
        modificarValvula(i);
    });

    const valvula = document.getElementById(configuracionValvulas[i].id);
    const tramo = document.getElementsByClassName(configuracionValvulas[i].claseTramo);

    valvula.addEventListener(('click'), () => {
        modificarValvula(i);
    });
};



const valvulaBomba = document.getElementById('valvula-bomba');

const deslizadorBomba = document.getElementById('deslizador-pasos-agua');
deslizadorBomba.value = "0";


function modificarBomba(valor) {
    const nuevoValor = parseInt(deslizadorBomba.value) + valor;

    if (nuevoValor < 0 || nuevoValor > 50) {
        return;
    }

    deslizadorBomba.value = nuevoValor.toString();


    if (nuevoValor > 0) {
        valvulaBomba.classList.add('abierta');
    } else {
        valvulaBomba.classList.remove('abierta');
    }
}


deslizadorBomba.addEventListener('change', (evento) => {
    if (parseInt(evento.target.value) > 0) {
        valvulaBomba.classList.add('abierta');
    } else {
        valvulaBomba.classList.remove('abierta');
    }
});


const botonMenosPasos = document.getElementById('menos-pasos-agua');
const botonMasPasos = document.getElementById('mas-pasos-agua');

botonMenosPasos.addEventListener('click', () => {
    modificarBomba(-1);
});

botonMasPasos.addEventListener('click', () => {
    modificarBomba(1);
});

const botonCronometro = document.getElementById('iniciar-cronometro');
const botonPararCronometro = document.getElementById('parar-cronometro');
const cronometro = document.getElementById('cronometro');

let intervaloCronometro = null;
let tiempoInicial = null;
let tiempoTranscurrido = 0; // Guarda el tiempo acumulado al pausar

function actualizarCronometro() {
    const tiempoActual = new Date().getTime();
    const mili = (tiempoActual - tiempoInicial) + tiempoTranscurrido;
    
    // Formato mm:ss.sss
    cronometro.innerHTML = new Date(mili).toISOString().slice(14, 22);
}

function iniciarCronometro() {
    // Si ya está corriendo, no hace nada
    if (intervaloCronometro !== null) return;

    tiempoInicial = new Date().getTime();
    actualizarCronometro();

    // Actualizamos cada 10ms para una visualización fluida
    intervaloCronometro = window.setInterval(actualizarCronometro, 10);
}

function pararCronometro() {
    if (intervaloCronometro === null) return;

    clearInterval(intervaloCronometro);
    intervaloCronometro = null;

    // Guardamos el tiempo transcurrido para poder reanudar luego
    tiempoTranscurrido += new Date().getTime() - tiempoInicial;
}

// Event Listeners
botonCronometro.addEventListener('click', iniciarCronometro);
botonPararCronometro.addEventListener('click', pararCronometro);

const video = document.getElementById('video-stream');