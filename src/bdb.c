/*
 * KAD Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* bdb.c - implementation bdb storeage
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 */
 
void (*db_cfn)(const DB_ENV *dbenv, const char *baton, const char *msg);
 
struct bdb_ops
{
    int (*open)(struct bdb* db);
    int (*create)(struct bdb* db);
	int (*recovery)(struct bdb* db);	
	int (*delete)(struct bdb* db);
	int (*move)(struct bdb* db);
	int (*clone)(struct bdb* db);
	int (*close)(struct bdb* db);
};

struct bdb
{
	DB_ENV *env;					/* db store enviromemt*/
	DB *db;							/* db store instance*/
	unsigned int flags;				/* db store flags*/
	char* home_dir;					/* db  enviromemt path*/
	char* data_dir;					/* db files store path*/
	size_t cache_size;				/* db memory cache file*/
	char* db_fname;				    /* db instance file name*/
	char* log_fname;				/* db log file name*/
	char* err_fname;				/* db log file file name*/
	FILE *err_file;				    /* db error file handle*/
	char* err_pfx					/* db log prefix string*/
	db_fn err_call;					/* db error callback func*/
	unsigned verbose				/* db log verbose level*/
	unsigned refcnt;				/* db reference counter*/
	struct bdb_ops *ops;			/* db  operation funcions*/
};

void bdb_dir_check(char* dir)
{
	struct stat st;
	asset(dir);
	
    /* ensure home directory exists */
	if (stat(dir, &st) == 0) {
		if (!S_ISDIR(st.st_mode))
			return -1; /*not dir*/
	}else{
        if (errno == ENOENT) { /*create dir*/
            if (mkdir(dir, S_IRUSR|S_IWUSR|S_IXUSR ) != 0)
                return -1;
        }else{ /*stat error*/
            return -1; 
        }
    }
	
	/* dir avaiable*/
	return 0;
}

int bdb_file_check(char* db_fname)
{
	/* check the db file access*/
    if (access(db_fname, R_OK) == -1) {
        printf("Not permission access %s.\n",db_fname);
        return -1;
    }
	return 0;
}

struct bdb* bdb_init(char *home_dir, char* data_dir,char* err_fname)	
{
	FILE *fp = NULL;
	struct bdb* bdb = NULL;
	
	/* check the parameters*/
	if(!home_dir || !data_dir || !err_fname)
		return -1;
	
	/*check the store bdb dir*/
	if(bdb_dir_check(home_dir) == -1)
		return -1;

	if(bdb_dir_check(data_dir) == -1)
		return -1;

    fp = fopen(err_fname, "w"));
	if(fp == NULL)
		return -1;
		
	/*alloc the storedb struct*/
	bdb = malloc(struct bdb);
	if(bdb == NULL)
		return -1;
		
	memset(bdb,0,sizeof(struct bdb));
	bdb->home_dir = strdup(home_dir);
	bdb->data_dir = strdup(data_dir);
	bdb->err_fname = strdup(err_fname);
	bdb->err_file = fp;
	bdb->cache_size = 5 * 1024 * 1024;
	bdb->verbose = 1;
	
	/* return bdb instance*/
	return bdb;
}

DB_ENV* bdb_env_setup(char *home, char *data_dir, char *errfp, FILE *progname)
{
	int ret;
	DB_ENV *dbenv = NULL;	
	
	/*
	* Create an environment and initialize it for additional error
	* reporting.
	*/
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		  fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		  return (NULL);
	}
	dbenv->set_errfile(dbenv, errfp);
	dbenv->set_errpfx(dbenv, progname);

	/* Set db verbose flags*/
    dbenv->set_verbose(dbenv, DB_VERB_CHKPOINT,
                (db->verbose & DB_VERB_CHKPOINT));
    dbenv->set_verbose(dbenv, DB_VERB_DEADLOCK,
                (db->verbose & DB_VERB_DEADLOCK));
    dbenv->set_verbose(dbenv, DB_VERB_RECOVERY,
                (db->verbose & DB_VERB_RECOVERY));
    dbenv->set_verbose(dbenv, DB_VERB_WAITSFOR,
                (db->verbose & DB_VERB_WAITSFOR));	
				
	/*
	* Specify the shared memory buffer pool cachesize: 5MB.
	* Databases are in a subdirectory of the environment home.
	*/
	if ((ret = dbenv->set_cachesize(dbenv, 0, 5 * 1024 * 1024, 0)) != 0) {
		  dbenv->err(dbenv, ret, "set_cachesize");
		  goto err;
	}
	if ((ret = dbenv->set_data_dir(dbenv, data_dir)) != 0) {
		  dbenv->err(dbenv, ret, "set_data_dir: %s", data_dir);
		  goto err;
	}

	/* Open the environment with full transactional support. */
	if ((ret = dbenv->open(dbenv, home, DB_CREATE |
	   DB_INIT_LOG | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN, 0)) != 0) {
		  dbenv->err(dbenv, ret, "environment open: %s", home);
		  goto err;
	}

	return (dbenv);

err: 
	(void)dbenv->close(dbenv, 0);
	return (NULL);
}

int bdb_env_create(struct bdb* bdb)
{
	DB_ENV *dbenv = NULL;	
	
	/* create and setup bdb enviroment*/
	dbenv = bdb_env_setup(bdb->home_dir,bdb->data_dir,
				  bdb->err_fname,bdb->err_file);				 
				  
	bdb->env = dbenv;
	return dbenv ? 0 : -1;
}

int bdb_create(struct bdb* bdb, char* db_name)
{
	int ret;
	if(!bdb || !db_name)
		return -1;
		
	return ret;
}

int bdb_close(struct bdb* bdb)
{
	int ret;
	DB *db = NULL;
	DB_ENV *dbenv = NULL;
	
	/* check the parameters*/
	if(!bdb) return -1;

	dbp = bdb->db;
	dbenv = bdb->env;	
	
	/* close the instance db */
	if( dbp != NULL ){
		if((ret = dbp->close(dbp, 0) !=0);
			printf("fail to close db,return %d\n",ret);
	}
	
	/* close the bdb enviromenet*/
	if( dbenv != NULL ){
		(void)dbenv->close(dbenv, 0);
	}
	
	return ret;
}

void bdb_destroy(struct bdb* bdb)
{
	/* check the parameters*/
	if(bdb == NULL)
		return;
	
	if(bdb->home_dir)
		free(bdb->home_dir);
	if(bdb->data_dir)
		free(bdb->data_dir);
	if(bdb->err_fname)
		free(bdb->err_fname);
	if(bdb->err_file)
		close(bdb->err_file);
		
	free(bdb);
}