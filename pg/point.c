/* $Id: point.c 22 2010-01-15 16:00:22Z olivier.chaussavoine $ */
/*
 openbarter - The maximum wealth for the minimum collective effort
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
/****************************************************************************
 * Les �tats d'un point de envit->points
 * un point stocke au moins l'offre et la marque
 * 	au parcours arriere, le point a la taille d'ob_tMarqueOffre
 * 	au parcours avant,
 * 		lorsque le chemin est vide, il stocke le stock � l'indice 0
 * 		sinon, il stocke le chemin.
 * 		sa taille est donn�e par getsizePoint
 *
 * ob_flux_cheminGetSize r�serve tjs au moins un ob_tNo pour y mettre le premier stock
 ***************************************************************************/
#include <point.h>
#include <flux.h>

size_t ob_point_getsizePoint(ob_tPoint *point) {
	int res;
	unsigned char _nb;

	return sizeof(ob_tPoint);
	// could also be:
	_nb = point->chemin.nbNoeud;
	res = sizeof(ob_tPoint);
	if (_nb <= 0) {
		res -= ((obCMAXCYCLE - 1) * sizeof(ob_tNo));
	} else if (_nb < obCMAXCYCLE) {
		res -= ((obCMAXCYCLE - (int) _nb) * sizeof(ob_tNo));
	}
	// else  res = sizeof(ob_tPoint);
	return res;
}
/****************************************************************************
 * initialise le point.
 * si le point est une source(si point.mo.av==1),
 * 	chemin contient un element: chemin=[point->mo.offre]
 * sinon, le chemin est vide: chemin=[]
 *
 * Entr�e: point->mo est d�j� d�fini. il est suppos� avoir au moins la taille
 * 	ob_tMarqueOffre. Le chemin stock� pr�c�demment est ignor�.
 * returns error:
 *	ob_point_CerIdInconsistant
 *	ob_point_CerStockEpuise
 *
 ***************************************************************************/
//TODO
int ob_point_getErrorOffreStock(ob_tNoeud *o, ob_tStock *s) {
	int ret = 0;

	if (o->stockId != s->sid) {
		ret = ob_point_CerOffreInconsistant;
		goto fin;
	}
	if (o->nF != s->nF) {
		ret = ob_point_CerOffreInconsistant;
		goto fin;
	}
	if (o->own != s->own) {
		ret = ob_point_CerOffreInconsistant;
		goto fin;
	}
	fin: return ret;
}

/* initializes the point.
 *  point->mo.offre is defined -it was a marqueOffre
 */
int ob_point_initPoint(ob_tPrivateTemp *privt, ob_tPoint *point) {
	int ret; //, coucheav;
	ob_tStock *pstock;
	DBT du_stock,ks_sid,ks_oid,ds_point;
	ob_tLoop loop;

	//bool _fast = privt->cflags && ob_flux_CFast;

	ob_flux_cheminVider(&point->chemin, privt->cflags);
	// place for the first stock
	pstock = ob_flux_cheminGetAdrFirstStock(&point->chemin);
	// get the stocktemps[point->mo.offre.stockId]
	obMtDbtpU(du_stock, pstock);
	//ob_point_voirNoeud(&point->mo.offre);
	obMtDbtpS(ks_sid, &point->mo.offre.stockId);
	ret = privt->stocktemps->get(privt->stocktemps, 0, &ks_sid, &du_stock, 0);
	if (ret) {
		obMTRACE(ret);
		// voirDBT(&point->mo.offre.stockId);
		goto fin;
	}
	// TODO if it is not found?? it should be

	if (privt->deposOffre)
		if (pstock->qtt == 0) {
			ret = ob_point_CerStockEpuise;
			goto fin;
		}

	if (point->mo.av.layer == 1) { // it is a source
		// put it into the chemin
		// 	chemin = [point->mo.offre]
		ret = ob_flux_cheminAjouterNoeud(&point->chemin, pstock,&point->mo.offre,&loop);
		if (ret)
			goto fin;
	}

	// save, not to redo it
	obMtDbtS(ks_oid, point->mo.offre.oid);
	obMtDbtpS(ds_point, point);
	ds_point.size = ob_point_getsizePoint(point);
	ret = privt->points->put(privt->points, 0, &ks_oid, &ds_point, 0);
	if (ret) {
		obMTRACE(ret);
		goto fin;
	}
	fin: return ret;
}
/*******************************************************************************
 pas_accepte
 lis envi.interdits, et indique si l'arc est accept�
 si Xoid==Yoid rend *refuse = true
 NB: envit sert a lire envit->txn
 *******************************************************************************/
// TODO appel, add txn
/*
int ob_point_pas_accepte(env, envt,txn, Xoid, Yoid, refuse)
	DB_ENV *env;DB_ENV *envt;DB_TXN *txn;ob_tId *Xoid, *Yoid;bool *refuse; {
	int ret = 0;
	DBT ks_fleche,du_interdit;

	ob_tPrivate *priv = env->app_private;
	//ob_tPrivateTemp *privt = envt->app_private;

	ob_tInterdit interdit;
	ob_tFleche rid;

	if (*Xoid == *Yoid) {
		ret = ob_point_CerRefusXY;
		goto fin;
	}
	obMtDbtS(ks_fleche, rid);
	obMtDbtU(du_interdit, interdit);

	// interdit.rid de type ob_tFleche
	rid.Xoid = *Xoid;
	rid.Yoid = *Yoid;
	//printf("rid= ");
	// ob_flux_voirDBT(stdout,&rid.Xoid,0);
	 // ob_flux_voirDBT(stdout,&rid.Yoid,1);
	//printf("txn=0x%x\n",privt->txn);
	ret = priv->interdits->get(priv->interdits, txn, &ks_fleche,
			&du_interdit, 0);
	if (!ret) { // it is refused, no error
		//ob_point_voirInterdit(&interdit);
		*refuse = true;
		goto fin;
	} else if (ret == DB_NOTFOUND) {
		*refuse = false;
		ret = 0;
	} else {
		obMTRACE(ret);
		goto fin;
	}
	fin: return (ret);
} */
/*******************************************************************************
 getPoint read the point.
 *******************************************************************************/
int ob_point_getPoint(DB *db, ob_tId *oid, ob_tPoint *point) {
	int ret;
	DBT ks_oid,du_point;

	obMtDbtpS(ks_oid, oid);
	obMtDbtpU(du_point, point);

	ret = db->get(db, 0, &ks_oid, &du_point, 0);
#ifndef NDEBUG // mise au point
	if (!ret) {
		if (du_point.size == sizeof(ob_tMarqueOffre)) {
			ret = ob_point_CerGetPoint;
			goto fin;
		}
		ret = ob_flux_cheminError(&point->chemin);
		if (ret)
			goto fin;
	}
#endif
	fin: return (ret);
}
/*******************************************************************************
 voirStockTemp visualise un envit->stocktemps d'indice sid
 *******************************************************************************/
/*static int _voirLeStockTemp(ob_tPrivateTemp *privt, ob_tId *sid) {
	int ret = 0;
	ob_tStock stock;
	DBT ks_sid,du_stock;

	obMtDbtpS(ks_sid, sid);
	obMtDbtU(du_stock, stock);
	ret
			= privt->stocktemps->get(privt->stocktemps, NULL, &ks_sid,
					&du_stock, 0);
	if (ret == DB_KEYEMPTY)
		printf("Stocktemps %lli not found.\n", *sid);
	else if (ret)
		obMTRACE(ret);
	else {
		obMTRACE(ret);
		ob_point_voirStock(&stock);
	}
	return ret;
}*/
/*******************************************************************************
 new_trait
 place le trait dans le i_graph 0
 *******************************************************************************/
int ob_point_new_trait(DB_ENV *envt, ob_tNoeud *offreX, ob_tNoeud *offreY) {
	ob_tTrait trait;
	int ret = 0;//, ret_t;
	ob_tPrivateTemp *privt = envt->app_private;
	DBT ds_trait,ks_fleche;

	obMtDbtS(ds_trait, trait);
	obMtDbtS(ks_fleche, trait.rid);

	trait.igraph = 0;
	trait.rid.Xoid = offreX->oid;
	trait.rid.Yoid = offreY->oid;

	ret = privt->traits->put(privt->traits, 0, &ks_fleche, &ds_trait, 0);
	if (ret)
		obMTRACE(ret);
	return ret;
}

/*******************************************************************************
 loop_trait
 traitement d'erreur lorsqu'un chemin est vu dans parcours_avant
 plus long que le plus long chemin de parcours arri�re, ce
 qui indique qu'il y a une boucle
 *******************************************************************************/
int ob_point_loop_trait(envit, offreX, offreY, couche, nbCouche)
	ob_tPrivateTemp *envit;ob_tNoeud *offreX, *offreY;int couche; // longueur du chemin dans parcours_avant
	int nbCouche; // longueur max pr�vue par parcours_arri�re
{
	int ret = 0;//, ret_t;
	//ob_tOwnId autheur;

	/*
	 syslog(LOG_DEBUG,
	 "trait o%is%i->o%is%i dans un cycle  couche=%i,nbCouche=%i\n",
	 offreX->oid,offreX->stockId,
	 offreY->oid,offreY->stockId,
	 couche,nbCouche);

	 if (envi.is_master) { // on interdit le trait
	 autheur=0;
	 ret = obinterdire(envit->txn,offreX->oid,offreY->oid,
	 autheur,obCBoucleDetectee);
	 if(ret) obMTRACE(ret,"");

	 memcpy(&envit->interdit.autheur,0,sizeof(ob_tUid));
	 envit->interdit.rid.Xoid = offreX->oid;
	 envit->interdit.rid.Yoid = offreY->oid;
	 envit->interdit.flags = obCBoucleDetectee;
	 }
	 */
	return (ret);
}

/******************************************************************************
 *
 * if sid!=0, Read the stock[sid] and put it into envit->stockTemps[sid]
 * updates privt->versionGg to the max of stock.version
 * when sid==0, put pstock in envit->stockTemps[0]
 *
 ******************************************************************************/
// TODO add txn for callings
/*
int ob_point_put_stocktemp(DB_ENV *env, DB_TXN *txn, ob_tStock *pstock) {
	int ret;

	ob_tPrivate *priv = env->app_private;
	ob_tPrivateTemp *privt = priv->envt->app_private;
	DBT ks_sid,du_stock;

	obMtDbtpS(ks_sid, &pstock->sid);
	obMtDbtpU(du_stock, pstock);

	if(pstock->sid != 0) {
		ret = priv->stocks->get(priv->stocks, txn, 
				&ks_sid, &du_stock, 0);
		if (ret) { obMTRACE(ret); goto fin; }

		if (privt->versionSg <pstock->version) {
			privt->versionSg = pstock->version;
		}
	}
	ret = privt->stocktemps->put(privt->stocktemps, 0, 
			&ks_sid, &du_stock, 0);
	if (ret) { obMTRACE(ret); goto fin; }

fin: 
	return ret;
} */
/* get the stock with the id pstock->id
*/
/*
int ob_point_get_stock(privt,pstock) 
	ob_tPrivateTemp *privt;
	ob_tStock *pstock;
{
	return 0;
} */
/*
int ob_point_put_stocktemp2(DB_ENV *envt, DB_TXN *txn, ob_tStock *pstock) {
	int ret;

	//ob_tPrivate *priv = env->app_private;
	//ob_tPrivateTemp *privt = priv->envt->app_private;
	ob_tPrivateTemp *privt = envt->app_private;

	obMDbtpS(ks_sid, &pstock->sid);
	obMDbtpU(du_stock, pstock);

	if(pstock->sid != 0) {
		// ret = priv->stocks->get(priv->stocks, txn, &ks_sid, &du_stock, 0);
		ret = ob_point_get_stock(privt,pstock);
		if (ret) { obMTRACE(ret); goto fin; }

		if (privt->versionSg < pstock->version) {
			// envit->versinSg < pstock->version
			privt->versionSg = pstock->version;
		}
	}
	ret = privt->stocktemps->put(privt->stocktemps, 0, 
			&ks_sid, &du_stock, 0);
	if (ret) { obMTRACE(ret); goto fin; }

fin: 
	return ret;
}
*/
void ob_point_voirStock(ob_tStock *ps) {
	printf("Stock :");
	ob_flux_MVoirDBT(&ps->sid);
	printf("\tnF ");
	ob_flux_MVoirDBT(&ps->nF);
	printf("\tqtt %lli\n", ps->qtt);
	printf("\town ");
	ob_flux_MVoirDBT(&ps->own);
	return;
}
void ob_point_voirInterdit(ob_tInterdit *pi) {
	printf("Int. :");
	ob_flux_voirDBT(stdout, &pi->rid.Xoid, 0);
	ob_flux_voirDBT(stdout, &pi->rid.Yoid, 1);
	printf("auth. :");
	ob_flux_voirDBT(stdout, &pi->autheur, 1);
	printf("flags :");
	printf("0x%x\n", pi->flags);
	return;
}
void ob_point_voirNoeud(ob_tNoeud *pn) {
	printf("Noeud:");
	ob_flux_MVoirDBT(&pn->oid);
	printf("\tsid ");
	ob_flux_MVoirDBT(&pn->stockId);
	printf("\tnR ");
	ob_flux_MVoirDBT(&pn->nR);
	printf("\tnF ");
	ob_flux_MVoirDBT(&pn->nF);
	printf("\tomega %f\n", pn->omega);
	printf("\town ");
	ob_flux_MVoirDBT(&pn->own);
	return;
}
/******************************************************************************/
/* retourne le dernier aid ins�r�
 * s'il n'existe pas, retourne 0
 *
 * S ob_tId *aid
 * */
// TODO voir le cas ou on a fait le tour de l'horloge
/*
 int ob_point_get_version(DB_TXN *txn,ob_tId *aid) {
 int ret,ret_t;

 obMDbtpU(ku_aid,aid);
 obMDbtR(dr_accord);

 DBC *c_accord = NULL;
 ret=envi.accords->cursor(envi.accords,txn,&c_accord,0);
 if (ret) { obMTRACE(ret); goto fin;}

 ret = c_accord->get(c_accord,&ku_aid,&dr_accord,DB_LAST);
 if (ret) {
 if (ret == DB_NOTFOUND) { ret = 0; aid=0;}
 else obMTRACE(ret);
 }
 fin:
 obMCloseCursor(c_accord);
 return(ret);
 }*/