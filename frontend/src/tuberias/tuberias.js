import '../style.css'
import './tuberias.css'


/* ============================================================
   PURGA INICIAL
   ============================================================ */

const overlayPurga =
    document.getElementById('overlay-purga');


const TIEMPO_PURGA = 630000; // 30 segundos


setTimeout(() => {

    overlayPurga.remove();

}, TIEMPO_PURGA);

/* ============================================================
   CONFIGURACIÓN
   ============================================================ */

const CAMARA_URL_1 = import.meta.env.VITE_CAMARA_URL_1;
const CAMARA_URL_2 = import.meta.env.VITE_CAMARA_URL_2;


import {
    conectarWebsocket,
    conectarCamara,
    ws
} from '../main.js';


/* ============================================================
   CÁMARAS Y WEBSOCKET
   ============================================================ */

const video_1 = document.getElementById('video-stream-1');
const video_2 = document.getElementById('video-stream-2');


window.addEventListener('DOMContentLoaded', () => {

    conectarCamara(CAMARA_URL_1, video_1);
    conectarCamara(CAMARA_URL_2, video_2);

    conectarWebsocket(1);
});


/* ============================================================
   FUNCIÓN GENERAL PARA CAMBIAR ESTADO DE BOTONES
   ============================================================ */

function cambiarEstadoBoton(boton, abierto) {

    if (abierto) {

        boton.classList.remove('apagado');
        boton.classList.add('abierto');

    } else {

        boton.classList.remove('abierto');
        boton.classList.add('apagado');
    }
}


/* ============================================================
   CAÑERÍAS
   ============================================================ */

const configuracionValvulas = [

    {
        id: 'valvula-1',
        claseTramo: 'tramo-can-pvc',
        valvulaEquipo: 19
    },

    {
        id: 'valvula-2',
        claseTramo: 'tramo-can-gal',
        valvulaEquipo: 20
    },

    {
        id: 'valvula-3',
        claseTramo: 'tramo-can-3',
        valvulaEquipo: 21
    },

    {
        id: 'valvula-4',
        claseTramo: 'tramo-can-4',
        valvulaEquipo: 22
    }
];


const botonesCaneria = [

    document.getElementById('bot-can-1'),
    document.getElementById('bot-can-2'),
    document.getElementById('bot-can-3'),
    document.getElementById('bot-can-4')
];


let estadoValvulaCan = [
    false,
    false,
    false,
    false
];


/* ============================================================
   ACTUALIZAR TRAMOS DEL SVG
   ============================================================ */

/*
   Un mismo elemento del SVG puede pertenecer a más de una
   cañería.

   Por ejemplo:

       class="tramo-can-pvc tramo-can-gal"

   En ese caso, el tramo debe permanecer azul mientras
   AL MENOS UNA de esas cañerías esté abierta.
*/

function actualizarTramos() {

    for (const configuracion of configuracionValvulas) {

        const tramos =
            document.getElementsByClassName(
                configuracion.claseTramo
            );


        for (let i = 0; i < tramos.length; i++) {

            let debeEstarActivo = false;


            /*
               Buscamos si alguna cañería abierta utiliza
               este mismo elemento SVG.
            */

            for (
                let j = 0;
                j < configuracionValvulas.length;
                j++
            ) {

                if (!estadoValvulaCan[j]) {
                    continue;
                }


                const clase =
                    configuracionValvulas[j].claseTramo;


                if (tramos[i].classList.contains(clase)) {

                    debeEstarActivo = true;

                    break;
                }
            }


            if (debeEstarActivo) {

                tramos[i].classList.add('activo');

            } else {

                tramos[i].classList.remove('activo');
            }
        }
    }
}


/* ============================================================
   MODIFICAR CAÑERÍA
   ============================================================ */

function modificarValvulaCan(numeroValvula) {

    const boton =
        botonesCaneria[numeroValvula];


    const valvula =
        document.getElementById(
            configuracionValvulas[numeroValvula].id
        );


    const abierta =
        !estadoValvulaCan[numeroValvula];


    estadoValvulaCan[numeroValvula] =
        abierta;


    /* --------------------------------------------------------
       BOTÓN
       -------------------------------------------------------- */

    cambiarEstadoBoton(
        boton,
        abierta
    );


    boton.textContent =
        abierta ? 'A' : 'C';


    /* --------------------------------------------------------
       COMANDO AL ARDUINO
       -------------------------------------------------------- */

    const valvulaEquipo =
        configuracionValvulas[numeroValvula]
            .valvulaEquipo
            .toString();


    const mensaje = {

        id_equipo: 1,

        cmd:
            valvulaEquipo +
            (abierta ? 'A' : 'C')
    };


    ws.send(JSON.stringify(mensaje));

    console.log(mensaje);


    /* --------------------------------------------------------
       SVG
       -------------------------------------------------------- */

    if (abierta) {

        valvula.classList.add('abierta');

    } else {

        valvula.classList.remove('abierta');
    }


    /*
       IMPORTANTE:

       No hacemos directamente:

           tramo.classList.remove('activo');

       porque podría ser un tramo compartido con otra
       cañería.

       Recalculamos todos los tramos.
    */

    actualizarTramos();
}


/* ============================================================
   EVENTOS CAÑERÍAS
   ============================================================ */

for (let i = 0; i < 4; i++) {

    const boton =
        botonesCaneria[i];


    boton.addEventListener(
        'click',
        () => {

            modificarValvulaCan(i);
        }
    );


    const valvula =
        document.getElementById(
            configuracionValvulas[i].id
        );


    valvula.addEventListener(
        'click',
        () => {

            modificarValvulaCan(i);
        }
    );
}


/* ============================================================
   BOMBA
   ============================================================ */

const botonBomba =
    document.getElementById('bot-bomba');


let bombaEncendida = false;


botonBomba.addEventListener(
    'click',
    () => {

        bombaEncendida =
            !bombaEncendida;


        /* Cambiar botón */

        cambiarEstadoBoton(
            botonBomba,
            bombaEncendida
        );


        botonBomba.textContent =
            bombaEncendida
                ? 'Encendida'
                : 'Apagada';


        /* Comando */

        const mensaje = {

            id_equipo: 1,

            cmd:
                bombaEncendida
                    ? 'P'
                    : 'A'
        };


        ws.send(
            JSON.stringify(mensaje)
        );


        console.log(mensaje);
    }
);


/* ============================================================
   REGULADOR DE CAUDAL
   ============================================================ */

const deslizadorBomba =
    document.getElementById(
        'desl-pasos-agua'
    );


deslizadorBomba.value = '0';


function modificarBomba(valor) {

    const nuevoValor =
        parseInt(
            deslizadorBomba.value
        ) + valor;


    if (
        nuevoValor < 0 ||
        nuevoValor > 1250
    ) {

        return;
    }


    const mensaje = {

        id_equipo: 1,

        cmd:
            valor >= 0
                ? 'M+' + valor.toString()
                : 'M' + valor.toString()
    };


    ws.send(
        JSON.stringify(mensaje)
    );


    console.log(mensaje);


    deslizadorBomba.value =
        nuevoValor.toString();
}


let valorBomba =
    Number(
        deslizadorBomba.value
    );


deslizadorBomba.addEventListener(
    'change',
    (evento) => {

        const nuevoValor =
            Number(
                evento.target.value
            );


        const pasos =
            nuevoValor -
            valorBomba;


        if (pasos === 0) {
            return;
        }


        const mensaje = {

            id_equipo: 1,

            cmd:
                pasos > 0
                    ? 'M+' + pasos.toString()
                    : 'M' + pasos.toString()
        };


        ws.send(
            JSON.stringify(mensaje)
        );


        console.log(mensaje);


        valorBomba =
            nuevoValor;
    }
);


/* ============================================================
   BOTONES + Y -
   ============================================================ */

const botonMenosPasos =
    document.getElementById(
        'menos-pasos-agua'
    );


const botonMasPasos =
    document.getElementById(
        'mas-pasos-agua'
    );


botonMenosPasos.addEventListener(
    'click',
    () => {

        modificarBomba(-1);
    }
);


botonMasPasos.addEventListener(
    'click',
    () => {

        modificarBomba(1);
    }
);


/* ============================================================
   TOMAS
   ============================================================ */

/*
   cañeria:

       0 -> Cañería 1
       1 -> Cañería 2
       2 -> Cañería 3
       3 -> Cañería 4
*/

const botonesToma = [

    {
        elemento:
            document.getElementById(
                'bot-toma-1'
            ),

        numeroValvula1: 1,
        numeroValvula2: 2,

        caneria: 0
    },

    {
        elemento:
            document.getElementById(
                'bot-toma-2'
            ),

        numeroValvula1: 3,
        numeroValvula2: 4,

        caneria: 1
    },

    {
        elemento:
            document.getElementById(
                'bot-diafragma'
            ),

        numeroValvula1: 5,
        numeroValvula2: 6,

        caneria: 2
    },

    {
        elemento:
            document.getElementById(
                'bot-tobera'
            ),

        numeroValvula1: 7,
        numeroValvula2: 8,

        caneria: 2
    },

    {
        elemento:
            document.getElementById(
                'bot-contraccion'
            ),

        numeroValvula1: 9,
        numeroValvula2: 10,

        caneria: 2
    },

    {
        elemento:
            document.getElementById(
                'bot-v-globo'
            ),

        numeroValvula1: 11,
        numeroValvula2: 12,

        caneria: 3
    },

    {
        elemento:
            document.getElementById(
                'bot-v-esclusa'
            ),

        numeroValvula1: 13,
        numeroValvula2: 14,

        caneria: 3
    },

    {
        elemento:
            document.getElementById(
                'bot-expansion'
            ),

        numeroValvula1: 15,
        numeroValvula2: 16,

        caneria: 3
    }
];


/* ============================================================
   MODIFICAR TOMA
   ============================================================ */

function modificarToma(numeroToma) {

    const toma =
        botonesToma[numeroToma];


    const caneria =
        toma.caneria;


    const estaAbierta =
        toma.elemento.classList.contains(
            'abierto'
        );


    /*
       Si la toma está cerrada y queremos abrirla,
       primero verificamos que su cañería esté abierta.
    */

    if (
        !estaAbierta &&
        !estadoValvulaCan[caneria]
    ) {

        console.log(
            'No se puede abrir la toma: la cañería está cerrada.'
        );

        return;
    }


    const nuevoEstado =
        !estaAbierta;


    /* --------------------------------------------------------
       BOTÓN
       -------------------------------------------------------- */

    cambiarEstadoBoton(
        toma.elemento,
        nuevoEstado
    );


    toma.elemento.textContent =
        nuevoEstado
            ? 'A'
            : 'C';


    /* --------------------------------------------------------
       COMANDOS
       -------------------------------------------------------- */

    const valvula1 =
        toma.numeroValvula1
            .toString();


    const valvula2 =
        toma.numeroValvula2
            .toString();


    const estado =
        nuevoEstado
            ? 'A'
            : 'C';


    const mensaje = {

        id_equipo: 1,

        cmd:
            valvula1 +
            estado +
            '\n' +
            valvula2 +
            estado
    };


    ws.send(
        JSON.stringify(mensaje)
    );


    console.log(mensaje);
}


/* ============================================================
   EVENTOS TOMAS
   ============================================================ */

for (
    let i = 0;
    i < botonesToma.length;
    i++
) {

    botonesToma[i]
        .elemento
        .addEventListener(
            'click',
            () => {

                modificarToma(i);
            }
        );
}


/* ============================================================
   CRONÓMETRO
   ============================================================ */

const valorCronometro =
    document.querySelector(
        '#cont-valor span'
    );


const botonIniciarCronometro =
    document.getElementById(
        'iniciar-cronometro'
    );


const botonPararCronometro =
    document.getElementById(
        'parar-cronometro'
    );


const botonRestablecerCronometro =
    document.getElementById(
        'restablecer-cronometro'
    );


/* ------------------------------------------------------------
   ESTADO
   ------------------------------------------------------------ */

let cronometroActivo = false;

let tiempoInicio = 0;

let tiempoAcumulado = 0;

let intervaloCronometro = null;


/* ------------------------------------------------------------
   FORMATEAR TIEMPO
   ------------------------------------------------------------ */

function formatearTiempo(tiempo) {

    const minutos =
        Math.floor(tiempo / 60000);

    const segundos =
        Math.floor(
            (tiempo % 60000) / 1000
        );

    const centesimas =
        Math.floor(
            (tiempo % 1000) / 10
        );


    return (
        minutos.toString().padStart(2, '0') +
        ':' +
        segundos.toString().padStart(2, '0') +
        ':' +
        centesimas.toString().padStart(2, '0')
    );
}


/* ------------------------------------------------------------
   ACTUALIZAR
   ------------------------------------------------------------ */

function actualizarCronometro() {

    let tiempoActual =
        tiempoAcumulado;


    if (cronometroActivo) {

        tiempoActual +=
            performance.now() -
            tiempoInicio;
    }


    valorCronometro.textContent =
        formatearTiempo(tiempoActual);
}


/* ------------------------------------------------------------
   INICIAR
   ------------------------------------------------------------ */

botonIniciarCronometro.addEventListener(
    'click',
    () => {

        if (cronometroActivo) {
            return;
        }


        cronometroActivo = true;


        tiempoInicio =
            performance.now();


        intervaloCronometro =
            setInterval(
                actualizarCronometro,
                10
            );


        /* Mostrar controles */

        botonIniciarCronometro.hidden = true;

        botonPararCronometro.hidden = false;

        botonRestablecerCronometro.hidden = false;
    }
);


/* ------------------------------------------------------------
   PARAR
   ------------------------------------------------------------ */

botonPararCronometro.addEventListener(
    'click',
    () => {

        if (!cronometroActivo) {
            return;
        }


        tiempoAcumulado +=
            performance.now() -
            tiempoInicio;


        cronometroActivo = false;


        clearInterval(
            intervaloCronometro
        );


        intervaloCronometro = null;


        actualizarCronometro();
    }
);


/* ------------------------------------------------------------
   RESTABLECER
   ------------------------------------------------------------ */

botonRestablecerCronometro.addEventListener(
    'click',
    () => {

        if (intervaloCronometro !== null) {

            clearInterval(
                intervaloCronometro
            );

            intervaloCronometro = null;
        }


        cronometroActivo = false;

        tiempoInicio = 0;

        tiempoAcumulado = 0;


        actualizarCronometro();


        /* Volver al estado inicial */

        botonIniciarCronometro.hidden = false;

        botonPararCronometro.hidden = true;

        botonRestablecerCronometro.hidden = true;
    }
);