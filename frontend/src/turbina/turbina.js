import '../style.css';
import './turbina.css';

import {
    conectarWebsocket,
    ws,
    conectarCamara
} from '../main.js';

const CAMARA_URL_1 = import.meta.env.VITE_CAMARA_URL_1;

// ============================================================
// CONFIGURACIÓN
// ============================================================

const ID_EQUIPO = 2;

const CAUDAL_MIN = 0;
const CAUDAL_MAX = 1300;

const ANGULO_MIN = 0;
const ANGULO_MAX = 11.5;

// ============================================================
// ELEMENTOS HTML
// ============================================================

const video = document.getElementById('video-stream');

const rpm = document.getElementById('valor-rpm');
const presionP4 = document.getElementById('valor-presion-p4');
const presionP20 = document.getElementById('valor-presion-p20');

const sliderCaudal = document.getElementById('slider-caudal');
const sliderAguja = document.getElementById('slider-aguja');

const bomba = document.getElementById('bot-bomba');

const botonMasCaudal = document.getElementById('bot-mas-caudal');
const botonMenosCaudal = document.getElementById('bot-menos-caudal');

const botonMasAguja = document.getElementById('bot-mas-aguja');
const botonMenosAguja = document.getElementById('bot-menos-aguja');

// ============================================================
// ESTADO LOCAL
// ============================================================

let bombaEncendida = false;

let posicionCaudal = 0;
let anguloAguja = 0;

// ============================================================
// INICIALIZACIÓN
// ============================================================

window.addEventListener('DOMContentLoaded', () => {

    conectarCamara(CAMARA_URL_1, video);

    conectarWebsocket(ID_EQUIPO);

    agregarEscuchaWs();

});

// ============================================================
// ENVÍO DE COMANDOS
// ============================================================

function enviarComando(cmd) {

    const mensaje = {
        id_equipo: ID_EQUIPO,
        cmd: String(cmd)
    };

    console.log('Enviando comando:', mensaje);

    if (!ws || ws.readyState !== WebSocket.OPEN) {
        console.error('WebSocket no conectado');
        return false;
    }

    ws.send(JSON.stringify(mensaje));

    return true;
}

// ============================================================
// BOMBA
// ============================================================

function actualizarBomba(encendida) {

    bombaEncendida = encendida;

    if (bombaEncendida) {

        bomba.innerText = 'Encendida';

        bomba.classList.remove('apagado');
        bomba.classList.add('encendido');

    } else {

        bomba.innerText = 'Apagada';

        bomba.classList.remove('encendido');
        bomba.classList.add('apagado');
    }
}

bomba.addEventListener('click', () => {

    const nuevoEstado = !bombaEncendida;

    const comando = nuevoEstado ? 'B ON' : 'B OFF';

    if (enviarComando(comando)) {
        actualizarBomba(nuevoEstado);
    }

});

// ============================================================
// CAUDAL - MOTOR 2
// ============================================================

// Python recibe M2:posición y lo convierte en 2+N o 2-N.

function moverMotor2(posicion) {

    const nuevaPosicion = Math.max(
        CAUDAL_MIN,
        Math.min(CAUDAL_MAX, Number(posicion))
    );

    if (!Number.isFinite(nuevaPosicion)) return;

    if (enviarComando(`M2:${nuevaPosicion}`)) {

        posicionCaudal = nuevaPosicion;

        sliderCaudal.value = posicionCaudal;
    }
}

sliderCaudal.addEventListener('change', (evento) => {

    moverMotor2(evento.target.value);

});

botonMasCaudal.addEventListener('click', () => {

    if (posicionCaudal >= CAUDAL_MAX) return;

    moverMotor2(posicionCaudal + 10);

});

botonMenosCaudal.addEventListener('click', () => {

    if (posicionCaudal <= CAUDAL_MIN) return;

    moverMotor2(posicionCaudal - 10);

});

// ============================================================
// AGUJA - SERVO
// ============================================================

// El Arduino debe haber recibido INIT antes de aceptar Sángulo.

function moverServo(angulo) {

    const nuevoAngulo = Math.max(
        ANGULO_MIN,
        Math.min(ANGULO_MAX, Number(angulo))
    );

    if (!Number.isFinite(nuevoAngulo)) return;

    const anguloFormateado = nuevoAngulo.toFixed(1);

    if (enviarComando(`S${anguloFormateado}`)) {

        anguloAguja = nuevoAngulo;

        sliderAguja.value = anguloAguja;
    }
}

sliderAguja.addEventListener('change', (evento) => {

    moverServo(evento.target.value);

});

botonMasAguja.addEventListener('click', () => {

    if (anguloAguja >= ANGULO_MAX) return;

    moverServo(
        Math.min(
            ANGULO_MAX,
            Math.round((anguloAguja + 0.1) * 10) / 10
        )
    );

});

botonMenosAguja.addEventListener('click', () => {

    if (anguloAguja <= ANGULO_MIN) return;

    moverServo(
        Math.max(
            ANGULO_MIN,
            Math.round((anguloAguja - 0.1) * 10) / 10
        )
    );

});

// ============================================================
// RECEPCIÓN DE MENSAJES
// ============================================================
function agregarEscuchaWs(intentos = 0) {

    if (!ws) {
        if (intentos < 50) {
            window.setTimeout(
                () => agregarEscuchaWs(intentos + 1),
                100
            );
        } else {
            console.error('[WebSocket] No se pudo obtener el WebSocket.');
        }

        return;
    }

    ws.addEventListener('message', (evento) => {

        let mensaje;

        try {
            mensaje = JSON.parse(evento.data);
        } catch {
            console.log(
                '[WebSocket] Mensaje de texto:',
                evento.data
            );
            return;
        }

        console.log('[WebSocket] Datos recibidos:', mensaje);

        if (
            mensaje.id_equipo !== undefined &&
            Number(mensaje.id_equipo) !== ID_EQUIPO
        ) {
            return;
        }

        // Presiones: formato que devuelve Python.
        if (mensaje.tipo === 'presion') {

            if (presionP4) {
                presionP4.textContent =
                    mensaje.presion_p4 == null
                        ? 'Sin datos'
                        : `${Number(mensaje.presion_p4).toFixed(2)} bar`;
            }

            if (presionP20) {
                presionP20.textContent =
                    mensaje.presion_p20 == null
                        ? 'Sin datos'
                        : `${Number(mensaje.presion_p20).toFixed(2)} bar`;
            }

            if (rpm && mensaje.rpm !== undefined) {
                rpm.textContent =
                    mensaje.rpm == null
                        ? 'Sin datos'
                        : Number(mensaje.rpm).toFixed(0);
            }

            return;
        }

        // Mensajes generales de los Arduinos.
        if (mensaje.tipo === 'mensaje') {
            console.log(
                `[${mensaje.origen ?? 'Arduino'}] ${mensaje.mensaje}`
            );
            return;
        }

        // Compatibilidad con mensajes de estado anteriores.
        if (mensaje.estado?.bomba !== undefined) {
            actualizarBomba(mensaje.estado.bomba.encendida);
        }

        console.debug('[WebSocket] Mensaje sin tipo reconocido:', mensaje);
    });

    console.info('[WebSocket] Escuchando mensajes.');
}