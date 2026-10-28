
import '../style.css';
import './golpe.css';

import {
    conectarWebsocket,
    conectarCamara,
    ws
} from '../main.js';

const ID_EQUIPO = 2;
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL_1;

const videoCaudal = document.getElementById('video-caudal');
const sliderCaudal = document.getElementById('slider-caudal');
const botonMenosCaudal = document.getElementById('bot-menos-caudal');
const botonMasCaudal = document.getElementById('bot-mas-caudal');

const botonBomba = document.getElementById('bot-bomba');
const botonCierreRapido = document.getElementById('bot-cerrar-rapido');
const inputVelocidadCierre = document.getElementById('input-velocidad-cierre');
const botonCierreLento = document.getElementById('bot-cerrar-lento');

const valorPresion = document.getElementById('valor-presion');
const canvasPresion = document.getElementById('canvas-presion');
const terminalMensajes = document.getElementById('terminal-mensajes');

let bombaEncendida = false;
let cierreRapidoActivado = false;

const historialPresion = [];
const MAX_PUNTOS = 100;

window.addEventListener('DOMContentLoaded', () => {
    if (CAMARA_URL && videoCaudal) {
        conectarCamara(CAMARA_URL, videoCaudal);
    }

    conectarWebsocket(ID_EQUIPO);
    esperarWebsocket();
    dibujarGrafica();
});

function esperarWebsocket(intentos = 0) {
    if (!ws) {
        if (intentos < 50) {
            window.setTimeout(
                () => esperarWebsocket(intentos + 1),
                100
            );
        } else {
            agregarMensajeTerminal('No se pudo obtener el WebSocket.');
        }

        return;
    }

    ws.addEventListener('message', recibirMensaje);

    ws.addEventListener('open', () => {
        agregarMensajeTerminal('WebSocket conectado.');
    });

    ws.addEventListener('close', () => {
        agregarMensajeTerminal('WebSocket desconectado.');
    });
}

function enviarComando(cmd) {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        agregarMensajeTerminal('No hay conexión WebSocket.');
        return false;
    }

    ws.send(JSON.stringify({
        id_equipo: ID_EQUIPO,
        cmd: String(cmd)
    }));

    console.log('[WebSocket] Comando enviado:', cmd);
    agregarMensajeTerminal(`Comando enviado: ${cmd}`);

    return true;
}

function recibirMensaje(evento) {
    let datos;

    try {
        datos = JSON.parse(evento.data);
    } catch {
        agregarMensajeTerminal(evento.data);
        return;
    }

    if (
        datos.id_equipo !== undefined &&
        Number(datos.id_equipo) !== ID_EQUIPO
    ) {
        return;
    }

    if (datos.tipo === 'presion') {
        actualizarPresion(datos);
        return;
    }

    if (datos.tipo === 'mensaje') {
        agregarMensajeTerminal(
            `[${datos.origen ?? 'Arduino'}] ${datos.mensaje}`
        );
        return;
    }

    agregarMensajeTerminal(JSON.stringify(datos));
}

function agregarMensajeTerminal(mensaje) {
    if (!terminalMensajes) return;

    const linea = document.createElement('div');

    linea.textContent =
        `${new Date().toLocaleTimeString()} - ${mensaje}`;

    terminalMensajes.appendChild(linea);

    while (terminalMensajes.children.length > 100) {
        terminalMensajes.firstElementChild.remove();
    }

    terminalMensajes.scrollTop = terminalMensajes.scrollHeight;
}

function cambiarEstadoBoton(
    boton,
    activado,
    textoActivado,
    textoDesactivado
) {
    if (!boton) return;

    boton.classList.toggle('abierto', activado);
    boton.classList.toggle('apagado', !activado);

    boton.textContent = activado
        ? textoActivado
        : textoDesactivado;
}

// BOMBA
botonBomba?.addEventListener('click', () => {
    const nuevoEstado = !bombaEncendida;
    const comando = nuevoEstado ? 'B ON' : 'B OFF';

    if (!enviarComando(comando)) return;

    bombaEncendida = nuevoEstado;

    cambiarEstadoBoton(
        botonBomba,
        bombaEncendida,
        'Encendida',
        'Apagada'
    );
});

// ELECTROVÁLVULA
botonCierreRapido?.addEventListener('click', () => {
    const nuevoEstado = !cierreRapidoActivado;
    const comando = nuevoEstado ? 'V ON' : 'V OFF';

    if (!enviarComando(comando)) return;

    cierreRapidoActivado = nuevoEstado;

    cambiarEstadoBoton(
        botonCierreRapido,
        cierreRapidoActivado,
        'Activado',
        'Cerrar'
    );
});

// CAUDAL: lo regula el motor 1.
// Se envía un número entre 0 y 1300 al backend.
function enviarCaudalDelSlider() {
    if (!sliderCaudal) return;

    const caudal = Number(sliderCaudal.value);

    if (
        !Number.isInteger(caudal) ||
        caudal < 0 ||
        caudal > 1300
    ) {
        agregarMensajeTerminal(
            'El caudal debe ser un entero entre 0 y 1300.'
        );
        return;
    }

    enviarComando(String(caudal));
}

botonMenosCaudal?.addEventListener('click', () => {
    if (!sliderCaudal) return;

    sliderCaudal.value = Math.max(
        Number(sliderCaudal.min || 0),
        Number(sliderCaudal.value) - 1
    );

    enviarCaudalDelSlider();
});

botonMasCaudal?.addEventListener('click', () => {
    if (!sliderCaudal) return;

    sliderCaudal.value = Math.min(
        Number(sliderCaudal.max || 1300),
        Number(sliderCaudal.value) + 1
    );

    enviarCaudalDelSlider();
});

sliderCaudal?.addEventListener('change', enviarCaudalDelSlider);

// CIERRE LENTO: motor paso a paso 3.
// Envía el comando "3C <velocidad>" al Arduino de motores.
botonCierreLento?.addEventListener('click', () => {
    if (!inputVelocidadCierre) {
        agregarMensajeTerminal(
            'No se encontró el input de velocidad de cierre.'
        );
        return;
    }

    const textoVelocidad = inputVelocidadCierre.value.trim();
    const velocidad = Number(textoVelocidad);

    if (
        textoVelocidad === '' ||
        !Number.isInteger(velocidad) ||
        velocidad <= 0
    ) {
        agregarMensajeTerminal(
            'Ingresá una velocidad entera mayor que cero.'
        );
        return;
    }

    const comando = `3C ${velocidad}`;

    if (enviarComando(comando)) {
        agregarMensajeTerminal(
            `Solicitado cierre lento del motor 3 a velocidad ${velocidad}.`
        );
    }
});

// PRESIÓN
function actualizarPresion(datos) {

    const p20 = convertirPresion(datos.presion_p20);

    if (valorPresion) {

        const textoP20 = p20 === null
            ? 'Sin datos'
            : `${p20.toFixed(2)} bar`;

        valorPresion.textContent =
            `${textoP20}`;
    }

    historialPresion.push({
        tiempo: Number(datos.tiempo_ms) || Date.now(),
        p20
    });

    if (historialPresion.length > MAX_PUNTOS) {
        historialPresion.shift();
    }

    dibujarGrafica();
}

function convertirPresion(valor) {
    if (
        valor === null ||
        valor === undefined ||
        valor === ''
    ) {
        return null;
    }

    const numero = Number(valor);

    return Number.isFinite(numero) ? numero : null;
}

// GRÁFICA EN TIEMPO REAL
function dibujarGrafica() {
    if (!canvasPresion) return;

    const contexto = canvasPresion.getContext('2d');

    if (!contexto) return;

    const ancho = canvasPresion.clientWidth || 400;
    const alto = 220;
    const escala = window.devicePixelRatio || 1;

    canvasPresion.width = ancho * escala;
    canvasPresion.height = alto * escala;
    canvasPresion.style.height = `${alto}px`;

    contexto.setTransform(escala, 0, 0, escala, 0, 0);
    contexto.clearRect(0, 0, ancho, alto);

    const margenIzquierdo = 45;
    const margenDerecho = 12;
    const margenSuperior = 20;
    const margenInferior = 28;

    const graficaAncho =
        ancho - margenIzquierdo - margenDerecho;

    const graficaAlto =
        alto - margenSuperior - margenInferior;

    const valores = historialPresion
        .flatMap(p => [p.p20])
        .filter(v => v !== null && Number.isFinite(v));

    contexto.font = '11px sans-serif';
    contexto.strokeStyle = '#888';
    contexto.fillStyle = '#888';
    contexto.lineWidth = 1;

    contexto.beginPath();
    contexto.moveTo(margenIzquierdo, margenSuperior);
    contexto.lineTo(
        margenIzquierdo,
        alto - margenInferior
    );
    contexto.lineTo(
        ancho - margenDerecho,
        alto - margenInferior
    );
    contexto.stroke();

    contexto.fillText('Presión (bar)', 5, 12);

    if (historialPresion.length < 2 || valores.length === 0) {
        contexto.fillText(
            'Esperando datos de presión...',
            margenIzquierdo + 10,
            margenSuperior + 20
        );
        return;
    }

    const maximo = Math.max(1, ...valores);
    const minimo = Math.min(0, ...valores);
    const rango = maximo - minimo || 1;

    // Marcas y etiquetas del eje Y.
    for (let i = 0; i <= 4; i++) {
        const valor = minimo + (rango * i / 4);

        const y = alto - margenInferior -
            (graficaAlto * i / 4);

        contexto.fillStyle = '#888';
        contexto.fillText(valor.toFixed(1), 5, y + 4);

        contexto.strokeStyle = '#ddd';
        contexto.beginPath();
        contexto.moveTo(margenIzquierdo, y);
        contexto.lineTo(ancho - margenDerecho, y);
        contexto.stroke();
    }

    function dibujarSerie(clave, color) {
        contexto.beginPath();
        contexto.strokeStyle = color;
        contexto.lineWidth = 2;

        let primerPunto = true;

        historialPresion.forEach((punto, indice) => {
            const valor = punto[clave];

            if (valor === null || !Number.isFinite(valor)) {
                primerPunto = true;
                return;
            }

            const x = margenIzquierdo +
                (indice / (historialPresion.length - 1)) *
                graficaAncho;

            const y = alto - margenInferior -
                ((valor - minimo) / rango) * graficaAlto;

            if (primerPunto) {
                contexto.moveTo(x, y);
                primerPunto = false;
            } else {
                contexto.lineTo(x, y);
            }
        });

        contexto.stroke();
    }

    dibujarSerie('p20', '#d04b38');


    contexto.fillStyle = '#d04b38';
    contexto.fillText('P20', margenIzquierdo + 35, 12);
}

window.addEventListener('resize', dibujarGrafica);
