
-- id,own,oid,qtt_requ,qua_requ,qtt_prov,qua_prov,qtt
CREATE TYPE yorder AS (
	type int,
	id int,
	own int,
	oid int, -- reference the order of the stock (can be id itself)
    qtt_requ int8,
    qua_requ text,
    qtt_prov int8,
    qua_prov text,
    qtt int8,
    
    pos_requ box, -- box (point(lat,lon),point(lat,lon))
	pos_prov box, -- box (point(lat,lon),point(lat,lon))
    dist	float8,
 	carre_prov box -- carre_prov @> pos_requ 
);

CREATE FUNCTION yflow_in(cstring)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_out(yflow)
RETURNS cstring
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE yflow (
	INTERNALLENGTH = variable, 
	INPUT = yflow_in,
	OUTPUT = yflow_out,
	ALIGNMENT = double
);
COMMENT ON TYPE yflow IS 'yflow ''[(type,id,oid,own,qtt_requ,qtt_prov,qtt,proba), ...]''';

CREATE FUNCTION yflow_get_maxdim()
RETURNS int
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_dim(yflow)
RETURNS int
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_show(yflow)
RETURNS cstring
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_to_json(yflow)
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_to_jsona(yflow)
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_init(yorder)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_grow_backward(yorder,yorder,yflow)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_grow_forward(yflow,yorder,yorder)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_finish(yorder,yflow,yorder)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_contains_oid(int,yflow)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_match(yorder,yorder)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_match_quality(text,text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_checktxt(text)
RETURNS int
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_checkquaownpos(text,text,point,text,point,float8)
RETURNS int
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;



--------------------------------------------------------------------------------
-- AGGREGATE ywolf_max(yflow) 
--------------------------------------------------------------------------------
CREATE FUNCTION yflow_maxg(yflow,yflow)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE AGGREGATE yflow_max(yflow)
(
sfunc = yflow_maxg,
stype = yflow,
initcond = '[]'
);

CREATE FUNCTION yflow_reduce(yflow,yflow,boolean)
RETURNS yflow
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_is_draft(yflow)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_to_matrix(yflow)
RETURNS int8[][]
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION yflow_qtts(yflow)
RETURNS int8[]
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


--------------------------------------------------------------------------------
-- earth
--------------------------------------------------------------------------------
CREATE FUNCTION earth_dist_points(point,point)
    RETURNS float8
    AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

/* cube are cube_s0 */
/*
CREATE FUNCTION earth_dist_cubes_s0(cube,cube)
    RETURNS float8
    AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;
*/

CREATE FUNCTION earth_get_square(point, float8)
    RETURNS box
    AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

/* same as box(point,point) */
/*
CREATE FUNCTION earth_point_to_box_s0(point)
    RETURNS box
    AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;
*/

CREATE FUNCTION earth_box_to_point(box)
    RETURNS point
    AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


