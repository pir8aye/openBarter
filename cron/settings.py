#!/usr/bin/python
import os
# settings of the deamon
WORKDIR = os.path.abspath("..") #"/home/olivier/project/dja/mp/"
PGSQLOBDIR = os.path.abspath("../pg") #"/home/olivier/project/pgsql/contrib/openBarter/"
MAXFD = 1024

#logger
LOGGINGDIR = os.path.join(WORKDIR,"logs/")
LOGGINGmaxBytes = 1000 * 1000
LOGGINGbackupCount = 5

# simulation
NBQUALITY = 3
NBOWNER = 5

#output
JSONDIR = os.path.join(WORKDIR,"media/json/")

#postgres database
DATABASE_NAME = 'ob'
DATABASE_USER = 'olivier'         
DATABASE_PASSWORD = '' 
DATABASE_HOST = 'localhost'     
DATABASE_PORT = '5432' 

#berkeleydb database
BDB_HOME = "~/project/DemoDir/openbarter"

def init():
	""" unused """
	if not os.path.exists(LOGGINGDIR):
		os.makedirs(LOGGINGDIR,0777)  
	if not os.path.exists(JSONDIR):
		os.makedirs(JSONDIR,0777)
		

        
