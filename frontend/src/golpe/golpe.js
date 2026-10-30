import '../style.css';
import './golpe.css';

import {
    conectarWebsocket,
    conectarCamara,
    ws
} from '../main.js';

const ID_EQUIPO = 2;
const CAMARA_URL = import.meta.env.VITE_CAMARA_URL_1;

// El firmware del motor 3 usa 1500 us como velocidad predeterminada.
const COMANDO_CERRAR_LENTO = '3C';
const COMANDO_ABRIR_LENTO = '3A';

const videoCaudal = document.getElementById('video-caudal');
const sliderCaudal = document.getElementById('slider-caudal');
const botonMenosCaudal = document.getElementById('bot-menos-caudal');
const botonMasCaudal = document.getElementById('bot-mas-caudal');

const botonBomba = document.getElementById('bot-bomba');
const botonCierreRapido = document.getElementById('bot-cerrar-rapido');
const botonCierreLento = document.getElementById('bot-cerrar-lento');

const valorPresionP4 = document.getElementById('valor-presion-p4');
const valorPresionP20 = document.getElementById('valor-presion-p20');

const canvasPresion = document.getElementById('canvas-presion');
const botonIniciarMedicion = document.getElementById('bot-iniciar-medicion');
const botonTerminarMedicion = document.getElementById('bot-terminar-medicion');
const estadoMedicion = document.getElementById('estado-medicion');

let bombaEncendida = false;
let cierreRapidoCerrado = false;
let cierreLentoCerrado = false;

let medicionActiva = false;
let inicioMedicion = null;
let datosMedicion = [];


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
            console.error('[WebSocket] No se pudo obtener el WebSocket.');
        }

        return;
    }

    ws.addEventListener('message', recibirMensaje);

    ws.addEventListener('open', () => {
        console.info('[WebSocket] Conectado.');
    });

    ws.addEventListener('close', () => {
        console.warn('[WebSocket] Desconectado.');
    });

    ws.addEventListener('error', (error) => {
        console.error('[WebSocket] Error:', error);
    });

}


function enviarComando(cmd) {

    if (!ws || ws.readyState !== WebSocket.OPEN) {
        console.error('[WebSocket] No hay conexión. No se envió:', cmd);
        return false;
    }

    const mensaje = {
        id_equipo: ID_EQUIPO,
        cmd: String(cmd)
    };

    ws.send(JSON.stringify(mensaje));

    console.log('[WebSocket] Comando enviado:', mensaje);

    return true;
}


function recibirMensaje(evento) {

    let datos;

    try {
        datos = JSON.parse(evento.data);
    } catch {
        console.log(
            '[WebSocket] Mensaje recibido (no JSON):',
            evento.data
        );
        return;
    }

    console.log('[WebSocket] Mensaje recibido:', datos);

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
        console.info(
            `[${datos.origen ?? 'Arduino'}] ${datos.mensaje}`
        );
        return;
    }

    console.debug(
        '[WebSocket] Mensaje sin tipo reconocido:',
        datos
    );

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


// ============================================================
// BOMBA: ARDUINO DE VÁLVULAS
// ============================================================

botonBomba?.addEventListener('click', () => {

    const nuevoEstado = !bombaEncendida;

    const comando = nuevoEstado
        ? 'B ON'
        : 'B OFF';

    if (!enviarComando(comando)) return;

    bombaEncendida = nuevoEstado;

    cambiarEstadoBoton(
        botonBomba,
        bombaEncendida,
        'Encendida',
        'Apagada'
    );

});


// ============================================================
// CIERRE RÁPIDO: ELECTROVÁLVULA
// ============================================================

botonCierreRapido?.addEventListener('click', () => {

    const nuevoEstadoCerrado = !cierreRapidoCerrado;

    const comando = nuevoEstadoCerrado
        ? 'V ON'
        : 'V OFF';

    if (!enviarComando(comando)) return;

    cierreRapidoCerrado = nuevoEstadoCerrado;

    cambiarEstadoBoton(
        botonCierreRapido,
        cierreRapidoCerrado,
        'Abrir',
        'Cerrar'
    );

});


// ============================================================
// CAUDAL: MOTOR 1
// Posición absoluta entre 0 y 1300
// ============================================================

function enviarCaudalDelSlider() {

    if (!sliderCaudal) return;

    const caudal = Number(sliderCaudal.value);

    if (
        !Number.isInteger(caudal) ||
        caudal < 0 ||
        caudal > 1300
    ) {
        console.error(
            '[Caudal] El valor debe ser un entero entre 0 y 1300.'
        );
        return;
    }

    // Python convierte M1:valor en 1+N o 1-N.
    enviarComando(`M1:${caudal}`);

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


sliderCaudal?.addEventListener(
    'change',
    enviarCaudalDelSlider
);


// ============================================================
// CIERRE LENTO: MOTOR 3
// 3C cierra y 3A abre.
// ============================================================

botonCierreLento?.addEventListener('click', () => {

    const nuevoEstadoCerrado = !cierreLentoCerrado;

    const comando = nuevoEstadoCerrado
        ? COMANDO_CERRAR_LENTO
        : COMANDO_ABRIR_LENTO;

    if (!enviarComando(comando)) return;

    cierreLentoCerrado = nuevoEstadoCerrado;

    cambiarEstadoBoton(
        botonCierreLento,
        cierreLentoCerrado,
        'Abrir',
        'Cerrar'
    );

});


// ============================================================
// RECEPCIÓN DE PRESIÓN
// ============================================================

function convertirPresion(valor) {

    if (
        valor === null ||
        valor === undefined ||
        valor === ''
    ) {
        return null;
    }

    const numero = Number(valor);

    return Number.isFinite(numero)
        ? numero
        : null;

}


function actualizarPresion(datos) {

    const p4 = convertirPresion(datos.presion_p4);
    const p20 = convertirPresion(datos.presion_p20);

    if (valorPresionP4) {

        valorPresionP4.textContent =
            p4 === null
                ? 'Sin datos'
                : `${p4.toFixed(2)} bar`;

    }

    if (valorPresionP20) {

        valorPresionP20.textContent =
            p20 === null
                ? 'Sin datos'
                : `${p20.toFixed(2)} bar`;

    }

    // Solo se almacenan muestras durante la medición.

    if (medicionActiva && inicioMedicion !== null) {

        const ahora = performance.now();

        datosMedicion.push({

            fecha: new Date().toISOString(),

            tiempo_s: (ahora - inicioMedicion) / 1000,

            presion_p4: p4,

            presion_p20: p20

        });

        dibujarGrafica();

    }

}


// ============================================================
// INICIO Y FIN DE MEDICIÓN
// ============================================================

botonIniciarMedicion?.addEventListener(
    'click',
    iniciarMedicion
);

botonTerminarMedicion?.addEventListener(
    'click',
    terminarMedicion
);


function iniciarMedicion() {

    if (medicionActiva) return;

    datosMedicion = [];

    inicioMedicion = performance.now();

    medicionActiva = true;

    if (botonIniciarMedicion) {
        botonIniciarMedicion.disabled = true;
    }

    if (botonTerminarMedicion) {
        botonTerminarMedicion.disabled = false;
    }

    if (estadoMedicion) {
        estadoMedicion.textContent = 'Medición en curso';
    }

    console.info(
        '[Medición] Comenzó la adquisición de datos.'
    );

    dibujarGrafica();

}


function terminarMedicion() {

    if (!medicionActiva) return;

    medicionActiva = false;

    if (botonIniciarMedicion) {
        botonIniciarMedicion.disabled = false;
    }

    if (botonTerminarMedicion) {
        botonTerminarMedicion.disabled = true;
    }

    if (estadoMedicion) {

        estadoMedicion.textContent =
            `Medición finalizada: ${datosMedicion.length} muestras`;

    }

    exportarMedicionCSV();

    console.info(
        `[Medición] Finalizada. Muestras: ${datosMedicion.length}.`
    );

    dibujarGrafica();

}


// ============================================================
// EXPORTACIÓN CSV
// ============================================================

function exportarMedicionCSV() {

    const encabezado = [

        'Fecha y hora',

        'Tiempo desde el inicio (s)',

        'Presion P4 (bar)',

        'Presion P20 (bar)'

    ];

    const filas = datosMedicion.map(muestra => [

        muestra.fecha,

        muestra.tiempo_s.toFixed(3),

        muestra.presion_p4 ?? '',

        muestra.presion_p20 ?? ''

    ]);

    const contenido = [encabezado, ...filas]

        .map(fila => fila.map(escaparCSV).join(';'))

        .join('\r\n');

    const archivo = new Blob(

        ['\uFEFF', contenido],

        { type: 'text/csv;charset=utf-8;' }

    );

    const url = URL.createObjectURL(archivo);

    const enlace = document.createElement('a');

    const fecha = new Date()
        .toISOString()
        .replace(/[:.]/g, '-');

    enlace.href = url;

    enlace.download = `medicion_presion_${fecha}.csv`;

    document.body.appendChild(enlace);

    enlace.click();

    enlace.remove();

    window.setTimeout(
        () => URL.revokeObjectURL(url),
        1000
    );

    console.info(
        `[Medición] Archivo CSV generado con ${datosMedicion.length} muestras.`
    );

}


function escaparCSV(valor) {

    const texto = String(valor ?? '');

    return `"${texto.replace(/"/g, '""')}"`;

}


// ============================================================
// GRÁFICA: P4 EN FUNCIÓN DEL TIEMPO
// ============================================================

function dibujarGrafica() {

    if (!canvasPresion) return;

    const contexto = canvasPresion.getContext('2d');

    if (!contexto) return;

    const ancho = Math.max(
        300,
        canvasPresion.clientWidth
    );

    const alto = Math.max(
        280,
        canvasPresion.clientHeight
    );

    const escala = window.devicePixelRatio || 1;

    canvasPresion.width = Math.round(ancho * escala);

    canvasPresion.height = Math.round(alto * escala);

    contexto.setTransform(
        escala,
        0,
        0,
        escala,
        0,
        0
    );

    contexto.clearRect(0, 0, ancho, alto);

    const margenIzquierdo = 58;
    const margenDerecho = 20;
    const margenSuperior = 28;
    const margenInferior = 48;

    const graficaAncho =
        ancho - margenIzquierdo - margenDerecho;

    const graficaAlto =
        alto - margenSuperior - margenInferior;

    contexto.font = '12px Lato, sans-serif';

    contexto.fillStyle = '#444';

    contexto.strokeStyle = '#999';

    contexto.lineWidth = 1;

    const muestras = datosMedicion;

    // Tomamos únicamente los valores de P4.

    const valores = muestras

        .map(m => m.presion_p4)

        .filter(
            v => v !== null && Number.isFinite(v)
        );

    if (muestras.length === 0 || valores.length === 0) {

        contexto.fillText(

            medicionActiva
                ? 'Esperando muestras de presión...'
                : 'Presioná "Empezar a medir" para iniciar el registro.',

            margenIzquierdo + 10,

            margenSuperior + 25

        );

        dibujarEjesVacios(
            contexto,
            ancho,
            alto,
            margenIzquierdo,
            margenSuperior,
            margenInferior,
            margenDerecho
        );

        return;

    }

    const tiempoMaximo = Math.max(
        10,
        muestras[muestras.length - 1].tiempo_s
    );

    const minimo = Math.min(0, ...valores);

    const maximo = Math.max(1, ...valores);

    const rango = maximo - minimo || 1;

    // Ejes y cuadrícula.

    for (let i = 0; i <= 5; i++) {

        const proporcion = i / 5;

        const y =
            margenSuperior +
            graficaAlto * (1 - proporcion);

        const valor =
            minimo + rango * proporcion;

        contexto.beginPath();

        contexto.strokeStyle = '#e0e3e7';

        contexto.moveTo(margenIzquierdo, y);

        contexto.lineTo(
            ancho - margenDerecho,
            y
        );

        contexto.stroke();

        contexto.fillStyle = '#555';

        contexto.textAlign = 'right';

        contexto.fillText(
            valor.toFixed(1),
            margenIzquierdo - 8,
            y + 4
        );

    }

    for (let i = 0; i <= 5; i++) {

        const tiempo = tiempoMaximo * i / 5;

        const x =
            margenIzquierdo +
            graficaAncho * i / 5;

        contexto.beginPath();

        contexto.strokeStyle = '#f0f0f0';

        contexto.moveTo(
            x,
            margenSuperior
        );

        contexto.lineTo(
            x,
            alto - margenInferior
        );

        contexto.stroke();

        contexto.fillStyle = '#555';

        contexto.textAlign = 'center';

        contexto.fillText(
            tiempo.toFixed(1),
            x,
            alto - margenInferior + 20
        );

    }

    contexto.textAlign = 'left';

    contexto.fillStyle = '#333';

    contexto.fillText(
        'Presión (bar)',
        8,
        16
    );

    // Dibujar P4.

    dibujarSerie(

        contexto,

        muestras,

        'presion_p4',

        '#2878c8',

        tiempoMaximo,

        minimo,

        rango,

        margenIzquierdo,

        margenSuperior,

        graficaAncho,

        graficaAlto,

        alto,

        margenInferior

    );

    contexto.font = '12px Lato, sans-serif';

    contexto.fillStyle = '#444';

    contexto.textAlign = 'center';

    contexto.fillText(

        'Tiempo (s)',

        margenIzquierdo + graficaAncho / 2,

        alto - 5

    );

}


function dibujarSerie(

    contexto,

    muestras,

    clave,

    color,

    tiempoMaximo,

    minimo,

    rango,

    margenIzquierdo,

    margenSuperior,

    graficaAncho,

    graficaAlto,

    alto,

    margenInferior

) {

    contexto.beginPath();

    contexto.strokeStyle = color;

    contexto.lineWidth = 2;

    let primerPunto = true;

    for (const muestra of muestras) {

        const valor = muestra[clave];

        if (
            valor === null ||
            !Number.isFinite(valor)
        ) {

            primerPunto = true;

            continue;

        }

        const x =
            margenIzquierdo +
            (muestra.tiempo_s / tiempoMaximo) * graficaAncho;

        const y =
            margenSuperior +
            graficaAlto -
            ((valor - minimo) / rango) * graficaAlto;

        if (primerPunto) {

            contexto.moveTo(x, y);

            primerPunto = false;

        } else {

            contexto.lineTo(x, y);

        }

    }

    contexto.stroke();

}


function dibujarEjesVacios(

    contexto,

    ancho,

    alto,

    margenIzquierdo,

    margenSuperior,

    margenInferior,

    margenDerecho

) {

    contexto.beginPath();

    contexto.strokeStyle = '#999';

    contexto.moveTo(
        margenIzquierdo,
        margenSuperior
    );

    contexto.lineTo(
        margenIzquierdo,
        alto - margenInferior
    );

    contexto.lineTo(
        ancho - margenDerecho,
        alto - margenInferior
    );

    contexto.stroke();

    contexto.fillStyle = '#555';

    contexto.textAlign = 'center';

    contexto.fillText(

        'Tiempo (s)',

        margenIzquierdo +
        (ancho - margenIzquierdo - margenDerecho) / 2,

        alto - 5

    );

}


window.addEventListener('resize', dibujarGrafica);