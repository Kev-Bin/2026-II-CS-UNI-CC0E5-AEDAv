
DROP TABLE IF EXISTS establecimientos CASCADE;
DROP TABLE IF EXISTS productos CASCADE;
DROP TABLE IF EXISTS usuarios CASCADE;

-- =====================================================

CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,
    nombre VARCHAR(100),
    correo VARCHAR(150),
    edad INTEGER
);

CREATE TABLE productos (
    id SERIAL PRIMARY KEY,
    codigo INTEGER,
    nombre VARCHAR(100),
    precio NUMERIC(10,2)
);

CREATE TABLE establecimientos (
    id SERIAL PRIMARY KEY,
    nombre VARCHAR(100),
    ubicacion POINT
);


-- =====================================================


INSERT INTO usuarios (nombre, correo, edad)
SELECT
    'Usuario ' || i,
    'usuario' || i || '@correo.com',
    18 + (i % 60)
FROM generate_series(1, 100000) AS i;


INSERT INTO productos (codigo, nombre, precio)
SELECT
    i,
    'Producto ' || i,
    (i % 1000) + 10.00
FROM generate_series(1, 100000) AS i;


INSERT INTO establecimientos (nombre, ubicacion)
SELECT
    'Establecimiento ' || i,
    POINT(
        -77.10 + random() * 0.20,
        -12.15 + random() * 0.20
    )
FROM generate_series(1, 100000) AS i;


-- =====================================================
-- B-TREE
-- =====================================================

CREATE INDEX idx_usuarios_correo
ON usuarios USING BTREE (correo);

SELECT *
FROM usuarios
WHERE correo = 'usuario50000@correo.com';

CREATE INDEX idx_usuarios_edad
ON usuarios USING BTREE (edad);

SELECT *
FROM usuarios
WHERE edad BETWEEN 20 AND 25;

-- =====================================================
-- HASH
-- =====================================================

CREATE INDEX idx_productos_codigo
ON productos USING HASH (codigo);

SELECT *
FROM productos
WHERE codigo = 50000;

-- =====================================================
-- GIST
-- =====================================================

CREATE INDEX idx_establecimientos_ubicacion
ON establecimientos USING GIST (ubicacion);

SELECT *
FROM establecimientos
WHERE ubicacion <-> POINT(-77.03, -12.05) < 0.01;
