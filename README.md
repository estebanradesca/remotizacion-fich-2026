Aplicación web de control de equipos de laboratorio para la Facultad de Ingeniería en Recursos Hídricos.
Experimento de Reynolds

        	
Estructura del proyecto:

- backend: Manejo de la API a la que se conecta la página web, control del arduino y servidores
  - app: Control del backend
    - api: API a la que se conecta la página web
      - routers: Operaciones de rutas
      - schemas: Modelos para la validación de los datos
      
    - db: base de datos
    - services: Lógica de la aplicación (control del arduino y recepción de comandos desde la página web)
    - tcp_arduino: Conexión TCP con el módulo Arduino
    - mediamtx: Servidor para la conversión de la transmisión de las cámaras IP
    
  - venv: Entorno virtual utilizado para la API
    
- frontend: Página web 

- config: Configuración