from fastapi import APIRouter


router = APIRouter(
    prefix="/reynolds",
    tags=["websocket"]
)


@router.
async def captura_reynolds(
    id_equipo: int,
    imagen, 
    caudal: float,
    temperatura: float);