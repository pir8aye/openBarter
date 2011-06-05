/* $Id: dbe.c 22 2010-01-15 16:00:22Z olivier.chaussavoine $ */
/*
 openbarter - The maximum wealth for the lowest collective effort
 Copyright (C) 2008 olivier Chaussavoine

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 olivier.chaussavoine@openbarter.org
 */
#include <dbe.h>
#include "openbarter.h"
//#include <errno.h>
#include <sys/stat.h>
//#include <utils.h>

static int point_get_nX(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static int point_get_nY(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static int
		point_get_mar(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static int
		point_get_mav(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static int point_get_stockId(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey);
static int trait_get_Xoid(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey);
static int trait_get_Yoid(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey);
static int trait_get_marque(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey);

static void errcallback(const DB_ENV *dbenv, const char *errpfx,
		const char *msg);

static int openBasesTemp(DB_ENV *envt, u_int32_t flagsdb);
static int closeBasesTemp(ob_tPrivateTemp *privt);
static int truncateBasesTemp(ob_tPrivateTemp *privt);

/******************************************************************************
 directory creation
 ******************************************************************************/
int ob_dbe_dircreate(char * path) {
	struct stat sb;
	int ret = 0;

	/* * If the directory exists, we're done. We do not further check
	 * the type of the file, DB will fail appropriately if it's the
	 * wrong type. */
	if (stat(path, &sb) == 0)
		goto fin;

	/* Create the directory, read/write/access owner only. */
	if (mkdir(path, S_IRWXU) != 0) {
		fprintf(stderr, "obarter: mkdir: %s: %s\n", path, strerror(errno));
		ret = ob_dbe_CerDirErr;
	}
	fin: return (ret);
}


/*******************************************************************************
 * open temporary env, next tables
 * flagsdb,
 * 	unused
 * path_db,
 * 	a string, absolute path of the database
 * penvt,
 * 	(out) a pointer to the new environment
 *
 * return 0 or an error
 *******************************************************************************/

/*
static void *_palloc(Size size) {
	return palloc(size);
}
*/
int ob_dbe_openEnvTemp(DB_ENV **penvt) {
	int ret = 0, ret_t;
	DB_ENV *envt = NULL;
	u_int32_t _flagsdb, _flagsenv;
	ob_tPrivateTemp *privt;

	ret = ob_makeEnvDir(openbarter_g.pathEnv);
	if(ret) return(ob_dbe_CerDirErr);

	ret = db_env_create(&envt, 0);
	if (ret) {
		obMTRACE(ret);
		goto abort;
	}

	envt->set_errcall(envt, errcallback);
	envt->set_errpfx(envt, "envit");
	/* supprimé car on est outofscape lors des appels
	ret = envt->set_alloc(envt,_palloc,repalloc,pfree);
	if (ret) {
		obMTRACE(ret);
		goto abort;
	} */
	// the size of the in memory cache
	ret = envt->set_cachesize(envt, 0, ob_dbe_CCACHESIZETEMP, 1);
	if (ret) {
		obMTRACE(ret);
		goto abort;
	}
	/* DB_INIT_MPOOL (docs/api_reference/C/envopen.html#open_DB_INIT_MPOOL)
	 * 	Initialize the shared memory buffer pool subsystem.
	 * 	This subsystem should be used whenever an application is using any Berkeley DB access method.
	 *
	 * DB_CREATE (docs/api_reference/C/envopen.html#open_DB_CREATE)
	 *	Cause Berkeley DB subsystems to create any underlying files, as necessary.
	 *
	 * DB_PRIVATE (docs/api_reference/C/envopen.html#open_DB_PRIVATE)
	 *	Allocate region memory from the heap instead of from memory backed by the filesystem or system shared memory.
	 *	This flag implies the environment will only be accessed by a single process (although that process may be multithreaded).
	 *	This flag has two effects on the Berkeley DB environment.
	 *	First, all underlying data structures are allocated from per-process memory instead of from shared memory
	 *	that is accessible to more than a single process. Second, mutexes are only configured to work between threads.
	 *	This flag should not be specified if more than a single process is accessing the environment
	 *	because it is likely to cause database corruption and unpredictable behavior.
	 *	For example, if both a server application and Berkeley DB utilities (for example, db_archive, db_checkpoint or db_stat)
	 *	are expected to access the environment, the DB_PRIVATE flag should not be specified.
	 */
	_flagsenv = DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE;
	ret = envt->open(envt, openbarter_g.pathEnv, _flagsenv, 0);
	if (ret) {
		obMTRACE(ret);
		goto abort;
	}

	privt = palloc(sizeof(ob_tPrivateTemp));
	if (!privt) {
		ret = ENOMEM;
		obMTRACE(ret);
		goto abort;
	}
	memset(privt, 0, sizeof(ob_tPrivateTemp));
	envt->app_private = privt;

	// how do we avoid the opening transaction ?
	// DB_PRIVATE => one process,
	// otherwise, region files are backed to file system
	_flagsdb = DB_CREATE | DB_TRUNCATE | DB_PRIVATE;
	// to activate  TXN
	// _flagsdb |= DB_AUTO_COMMIT;

	ret = openBasesTemp(envt, _flagsdb);
	if (ret)
		goto abort;

	*penvt = envt;
	return 0;

abort:
	if (envt != NULL) {
		ret_t = ob_dbe_closeEnvTemp(envt);
		if (!ret)
			ret = ret_t;
		*penvt = NULL;
	}
	return (ret);
}

/******************************************************************************
 close the temporary environment
 envt
 a pointer to the environment to be closed
 returns 0 or an error
 ******************************************************************************/
int ob_dbe_closeEnvTemp(DB_ENV *envt) {
	int ret = 0, ret_t;

	ob_tPrivateTemp *privt = envt->app_private;
	if (privt != NULL) {
		ret = closeBasesTemp(privt);
		pfree(privt);
		envt->app_private = NULL;
	}
	ret_t = envt->close(envt, 0);
	if (ret_t != 0 && ret == 0)
		ret = ret_t;
	// elog( ERROR,"Appel de ob_rmPath avec %s",openbarter_g.pathEnv );
	ob_rmPath(openbarter_g.pathEnv,true);
	return ret;
}

/******************************************************************************
 reset the temporary environment
	when **envt == NULL creates the envt and open batabases
	else truncate databases
 returns 0 or an error
 ******************************************************************************/
int ob_dbe_resetEnvTemp(DB_ENV **penvt) {
	int ret;

	if(*penvt == NULL) {
		ret = ob_dbe_openEnvTemp(penvt);
	} else {
		ret = truncateBasesTemp((*penvt)->app_private);
	}
	return ret;
}

static int open_db(DB_ENV *env, bool is_secondary, DB *db, char *name,
		DBTYPE type, u_int32_t flags, int(*bt_compare_fcn)(DB *db,
				const DBT *dbt1, const DBT *dbt2), int(*dup_compare_fcn)(
				DB *db, const DBT *dbt1, const DBT *dbt2)) {
	int ret, _flags;
	char *pname = name;

	if (bt_compare_fcn) {
		ret = db->set_bt_compare(db, bt_compare_fcn);
		if (ret) {
			obMTRACE(ret);
			goto err;
		}
	}
	if (dup_compare_fcn) {
		ret = db->set_dup_compare(db, dup_compare_fcn);
		if (ret) {
			obMTRACE(ret);
			goto err;
		}
	}

	if (is_secondary) {
		ret = db->set_flags(db, DB_DUPSORT);
		if (ret) {
			obMTRACE(ret);
			goto err;
		}
		_flags = flags & ~DB_TRUNCATE;
	} else
		_flags = flags;

	/* DB->open(DB *db, DB_TXN *txnid, const char *file, const char *database, DBTYPE type, u_int32_t flags, int mode);
	 * Whether other threads of control can access this database is driven entirely by whether the database parameter is set to NULL.
	 *
	 */
	ret = db->open(db, NULL, pname, NULL, type, _flags | DB_PRIVATE, 0644);
	if (ret) {
		obMTRACE(ret);
		goto err;
	}
	/*
	if (_flags & DB_PRIVATE) {
		// if DB_PRIVATE, region file resides in memory
		ret = db->open(db, NULL, NULL, pname, type, _flags, 0644);
		if (ret) {
			obMTRACE(ret);
			goto err;
		}
	} else {
		ret = db->open(db, NULL, pname, NULL, type, _flags, 0644);
		if (ret) {
			obMTRACE(ret);
			goto err;
		}
	}
	*/
	return 0;
	err: return (ret);
}



/*******************************************************************************
 * temporary environment
 ******************************************************************************/
static int createBasesTemp(ob_tPrivateTemp *privt, DB_ENV *envt) {
	int ret = 0;
	if (!privt) {
		ret = ob_dbe_CerPrivUndefined;
		goto fin;
	}

	if ((ret = db_create(&privt->px_traits, envt, 0)) != 0) {
		privt->px_traits = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->py_traits, envt, 0)) != 0) {
		privt->py_traits = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->m_traits, envt, 0)) != 0) {
		privt->m_traits = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->traits, envt, 0)) != 0) {
		privt->traits = NULL;
		goto fin;
	}

	if ((ret = db_create(&privt->vx_points, envt, 0)) != 0) {
		privt->vx_points = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->vy_points, envt, 0)) != 0) {
		privt->vy_points = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->mar_points, envt, 0)) != 0) {
		privt->mar_points = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->mav_points, envt, 0)) != 0) {
		privt->mav_points = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->st_points, envt, 0)) != 0) {
		privt->st_points = NULL;
		goto fin;
	}
	if ((ret = db_create(&privt->points, envt, 0)) != 0) {
		privt->points = NULL;
		goto fin;
	}

	if ((ret = db_create(&privt->stocktemps, envt, 0)) != 0) {
		privt->stocktemps = NULL;
		goto fin;
	}

	return 0;
	fin: return (ret);
}

static int openBasesTemp(DB_ENV *envt, u_int32_t flagsdb) {
	int ret;

	ob_tPrivateTemp *privt = envt->app_private;
	if (!privt) {
		ret = ob_dbe_CerPrivUndefined;
		return ret;
	}

	// provisoire
	ret = createBasesTemp(privt, envt);
	if(ret) goto fin;

	// traits
	ret = open_db(envt, false, privt->traits, "traits", DB_BTREE,flagsdb, NULL, NULL);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->m_traits, "m_traits",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;

	ret = privt->traits->associate(privt->traits, 0, privt->m_traits,trait_get_marque, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->px_traits, "px_traits",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->traits->associate(privt->traits, 0, privt->px_traits,trait_get_Xoid, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->py_traits,"py_traits",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->traits->associate(privt->traits, 0, privt->py_traits,trait_get_Yoid, 0);
	if(ret) goto fin;

	//points
	ret = open_db(envt, false, privt->points,"points", DB_BTREE,flagsdb, NULL, NULL);
	if(ret) goto fin;
	// the size of points is not constant

	ret = open_db(envt, true, privt->mar_points, "mar_points",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->points->associate(privt->points, 0, privt->mar_points,point_get_mar, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->mav_points, "mav_points",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->points->associate(privt->points, 0, privt->mav_points,point_get_mav, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->vx_points, "vx_points",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->points->associate(privt->points, 0, privt->vx_points,point_get_nX, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->vy_points, "vy_points",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->points->associate(privt->points, 0, privt->vy_points,point_get_nY, 0);
	if(ret) goto fin;

	ret = open_db(envt, true, privt->st_points, "st_points",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;
	ret = privt->points->associate(privt->points, 0, privt->st_points,point_get_stockId, 0);
	if(ret) goto fin;

	// stocktemps
	ret = open_db(envt, false, privt->stocktemps, "stocktemps",DB_BTREE, flagsdb, NULL, NULL);
	if(ret) goto fin;

	return 0;
fin:
	//syslog(LOG_DEBUG,"Erreur a l'ouverture d'envit");
	(void) closeBasesTemp(privt);
	return (ret);
}
/******************************************************************************/
#define ob_dbe_MCloseBase(base) do { if ((base)!=NULL){ \
		ret_t = (base)->close((base),0); \
		if( ret_t) { \
			if(!ret) ret = ret_t; \
		} \
		(base) = NULL; \
	} else elog(ERROR,"batabase is null- could not close"); \
	} while (0)
#define ob_dbe_MTruncateBase(base) do { if ((base)!=NULL) { \
		ret_t = (base)->truncate((base),NULL,&cnt,0); \
		if( ret_t) { \
			if(!ret) ret = ret_t; \
		} \
	} else elog(ERROR,"batabase is null - could not truncate"); \
	} while(0)
/******************************************************************************/
/******************************************************************************/
static int closeBasesTemp(ob_tPrivateTemp *privt) {
	int ret = 0, ret_t;
	u_int32_t cnt;
	
	if (!privt) {
		ret = ob_dbe_CerPrivUndefined;
		return ret;
	}
	ob_dbe_MTruncateBase(privt->traits);
	// secondary index are also truncated

	ob_dbe_MCloseBase(privt->px_traits);
	ob_dbe_MCloseBase(privt->py_traits);
	ob_dbe_MCloseBase(privt->m_traits);
	ob_dbe_MCloseBase(privt->traits);

	ob_dbe_MTruncateBase(privt->points);
	//
	ob_dbe_MCloseBase(privt->vx_points);
	ob_dbe_MCloseBase(privt->vy_points);
	ob_dbe_MCloseBase(privt->mar_points);
	ob_dbe_MCloseBase(privt->mav_points);
	ob_dbe_MCloseBase(privt->st_points);
	ob_dbe_MCloseBase(privt->points);

	ob_dbe_MTruncateBase(privt->stocktemps);
	//
	ob_dbe_MCloseBase(privt->stocktemps);
	// stat_env(envit->env,0);

	return (ret);
}
/******************************************************************************/
static int truncateBasesTemp(ob_tPrivateTemp *privt) {
	int ret = 0, ret_t;
	u_int32_t cnt;

	if (!privt) {
		ret = ob_dbe_CerPrivUndefined;
		return ret;
	}
	ob_dbe_MTruncateBase(privt->traits);
	// secondary index are also truncated
	ob_dbe_MTruncateBase(privt->points);
	ob_dbe_MTruncateBase(privt->stocktemps);

	return (ret);
}
/*******************************************************************************
 Callback error
 *******************************************************************************/
static void errcallback(const DB_ENV *dbenv, const char *errpfx,
		const char *msg) {

#ifdef obCTESTCHEM
	fprintf(stderr, "ob>%s> %s\n", errpfx, msg);
#else
	elog(ERROR, "ob>%s> %s\n", errpfx, msg);
#endif
	return;
}

/*******************************************************************************
 Fonctions d'extraction de clef sur l'evt temporaire
 *******************************************************************************/
static int point_get_nX(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey) {
	ob_tPoint *point;
	// risque de bp d'alignement
	point = pdata->data;

	memset(skey, 0, sizeof(DBT));
	skey->data = &(point->mo.offre.nR);
	skey->size = sizeof(ob_tQuaId);
	return (0);
}
static int point_get_nY(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey) {
	ob_tPoint *point;

	point = pdata->data;
	memset(skey, 0, sizeof(DBT));
	skey->data = &(point->mo.offre.nF);
	skey->size = sizeof(ob_tQuaId);
	return (0);
}

static int point_get_mar(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey) {
	ob_tPoint *point;
	point = pdata->data;
	
	memset(skey, 0, sizeof(DBT));
	skey->data = &(point->mo.ar);
	skey->size = sizeof(ob_tMar); 
	return (0);
} 

static int point_get_mav(DB *sdbp, const DBT *pkey, const DBT *pdata, DBT *skey) {
	ob_tPoint *point;
	point = pdata->data;
	
	memset(skey, 0, sizeof(DBT));
	skey->data = &(point->mo.av);
	skey->size = sizeof(ob_tMar); 
	return (0);
}	

static int point_get_stockId(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey) {
	ob_tPoint *point;

	point = pdata->data;
	memset(skey, 0, sizeof(DBT));
	skey->data = &(point->mo.offre.stockId);
	skey->size = sizeof(ob_tId);
	return (0);
}

static int trait_get_Xoid(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey) {
	ob_tTrait *trait;
	trait = pdata->data;

	memset(skey, 0, sizeof(DBT));
	skey->data = &(trait->rid.Xoid);
	skey->size = sizeof(ob_tId);
	return (0);
}

static int trait_get_Yoid(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey) {
	ob_tTrait *trait;
	trait = pdata->data;

	memset(skey, 0, sizeof(DBT));
	skey->data = &(trait->rid.Yoid);
	skey->size = sizeof(ob_tId);
	return (0);
}

static int trait_get_marque(DB *sdbp, const DBT *pkey, const DBT *pdata,
		DBT *skey) {
	ob_tTrait *trait;

	trait = pdata->data;

	memset(skey, 0, sizeof(DBT));
	skey->data = &(trait->igraph);
	skey->size = sizeof(int);
	return (0);
}

void ob_dbe_resetStock(ob_tStock *pstock) {
	memset(pstock, 0, sizeof(ob_tStock));
	pstock->sid = 0;
	pstock->own = 0;
	// ob_tQtt qtt
	pstock->nF = 0;
	pstock->version = 0;
	return;
}

void ob_dbe_resetNoeud(ob_tNoeud *pnoeud) {
	memset(pnoeud, 0, sizeof(ob_tNoeud));
	pnoeud->oid = 0;
	pnoeud->stockId = 0;
	// double omega;
	pnoeud->nR = 0;
	pnoeud->nF = 0;
	pnoeud->own = 0;
	return;
}

void ob_dbe_resetFleche(ob_tFleche *pfleche) {
	memset(pfleche, 0, sizeof(ob_tFleche));
	pfleche->Xoid  = 0;
	pfleche->Xoid = 0;
	return;
}
void ob_dbe_resetTrait(ob_tTrait *ptrait) {
	memset(ptrait, 0, sizeof(ob_tTrait));
	ob_dbe_resetFleche(&ptrait->rid);
	// int igraph
	return;
}
void ob_dbe_resetMarqueOffre(ob_tMarqueOffre *pmo) {
	memset(pmo, 0, sizeof(ob_tMarqueOffre));
	ob_dbe_resetNoeud(&pmo->offre);
	// int ar.layer,ar.igraph,av.layer,av.igraph
	return;
}
