/*
 * DHT Cache
 * Copyright (c), 2008, GuangFu, 
 * 
* store.c - implementation db storeage
 * 
* Author(s): wxlin  <linweixuangz@126.com>
 * 
 */
 
void (*db_cfn)(const DB_ENV *dbenv, const char *baton, const char *msg);
 
struct store_ops
{
    int (*open)(struct storedb* db);
    int (*create)(struct storedb* db);
	int (*recovery)(struct storedb* db);
	int (*clone)(struct storedb* db);
	int (*close)(struct storedb* db);
};

struct storedb
{
	DB_ENV *db_env;					/* db store enviromemt*/
	DB *db;							/* db store instance*/
	unsigned int db_flags;			/* db store flags*/
	char* home_dir;					/* db  enviromemt path*/
	char* data_dir;					/* db files store path*/
	size_t cache_size;				/* db memory cache file*/
	char* db_fname;				    /* db instance file name*/
	char* log_fname;				/* db log file handle name*/
	char* err_fname;				/* db log file file name*/
	FILE *err_file;				    /* db error file handle*/
	char* err_pfx					/* db log prefix string*/
	db_fn err_call;					/* db error callback func*/
	unsigned verbose				/* db log verbose level*/
	unsigned refcnt;				/* db reference counter*/
	struct store_ops *db_ops;		/* db  operation funcions*/
};

struct storedb
{
};

void db_dir_check(char* dir)
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

int db_file_check(struct storedb* db)
{
	/* check the db file access*/
    if (access(db->db_fname, R_OK) == -1) {
        printf("Not permission access %s.\n",db->db_fname);
        return -1;
    }
	return 0;
}

void db_error_callback(const DB_ENV *dbenv, const char *baton, const char *msg)
{

}

int db_store_config(struct storedb* db)
{

	return 0;
}

struct storedb* db_store_create(char *home_dir, char* data_dir,char* err_fname)
{
	FILE *fp;
	struct storedb* db;
	
	/* check the parameters*/
	assert(home_dir);
	assert(data_dir);
	assert(err_fname);
	
	/*check the storedb dir*/
	if(db_dir_check(home_dir) == -1)
		return -1;

	if(db_dir_check(data_dir) == -1)
		return -1;

    fp = fopen(err_fname, "w"));
	if(fp == NULL)
		return -1;
		
	/*alloc the storedb struct*/
	db = malloc(struct storedb);
	if(db == NULL)
		return -1;
		
	memset(db,0,sizeof(struct storedb));
	db->home_dir = strdup(home_dir);
	db->data_dir = strdup(data_dir);
	db->err_fname = strdup(err_fname);
	db->err_file = fp;
	db->cache_size = 5 * 1024 * 1024;
	db->verbose = 1;
	
	/* return store instance*/
	return db;
}

int db_store_setup(struct storedb* db)
{
	int ret;
	DB_ENV *dbenv;
	
	/* Create an environment and initialize. */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(errfp, "%s\n", db_strerror(ret));
		return (NULL);
	}
	
	/* Set error handle for reporting*/
	dbenv->set_errcall(dbenv, db->err_call);
	dbenv->set_errfile(dbenv, db->err_file);
	dbenv->set_errpfx(dbenv, db->err_fname);
	
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
	if ((ret = dbenv->set_cachesize(dbenv, 0, db->cache_size, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		goto err;
	}
	
	if ((ret = dbenv->set_data_dir(dbenv, db->data_dir)) != 0) {
		dbenv->err(dbenv, ret, "set_data_dir: %s", db->data_dir);
		goto err;
	}
	
	/* Open the environment with full transactional support. */
	if ((ret = dbenv->open(dbenv, db->home_dir, DB_CREATE | DB_INIT_LOG 
		| DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN, 0)) != 0) 
	{
		dbenv->err(dbenv, ret, "environment open: %s", db->home_dir);
		goto err;
	}	
	
	db->dbenv = dbenv;
	return 0;
	
err: 
	dbenv->close(dbenv,0);
	return -1;
}


int db_db_create(struct storedb* db,char* db_name)
{
	assert(db);	
	return 0;
}

int db_store_close(struct storedb* db)
{
	assert(db);
	dbenv->close(dbenv,0);
	return 0;
}

void db_store_destroy(struct storedb* db)
{
	assert(db);
	free(db);
}

int put_key_data(DB *db, void *key_val, int key_len, void *data_val, int data_len)
{
	int ret;
	DBT key;
	DBT data;	

	memset(&key,0,sizeof(key));
	memset(&data,0,sizeof(data));

	key.data = (void*)key_val;
	key.size = key_len;
	data.data = (void*)data_val;
	data.size = data_len;

	ret = db->put(table,NULL,&key,&data,0);
	if(ret == 0){
		/* put ok */
		return 0;
	}else if(ret==DB_RUNRECOVERY){
		fprintf(stderr,"put_key_data: DB recovery must be performed now. Start db_recover.\n");
		return 1;
	}else{
		fprintf(stderr,"put_key_data: unknown return code: %d\n",ret);
		dbenv->err(dbenv,ret,"db_appinit(set_key_data) fails");
		return 1;
	}
	return -1;
}

int del_key_data(DB *db, const void *key_val, const int key_len)
{
	int ret;
	DBT key;
	
	memset(&key,0,sizeof(key));
	key.data = (void*)key_val;
	key.size = key_len;

	ret = db->del(db,NULL,&key,0);
	if (ret == 0) {
		/* del ok */
		return 0;
	}else if(ret==DB_NOTFOUND){
		/* del ok */
		return 0;
	}else if(ret==DB_RUNRECOVERY){
		fprintf(stderr,"del_key_data: DB recovery must be performed now. Start db_recover.\n");
		return 1;
	}else{
		fprintf(stderr,"del_key_data: unknown return code: %d\n",ret);
		return 1;
	}
	
	return -1;
}

void db_query_begin(void **cookie, uint32_t flags)
{
	DBC             *dbc;
	uint32_t        dbflags;
	int             rc;

	assert(NULL != cookie);
	assert(NULL != dbp);

	dbflags = 0;
	if ( ! (flags & NLPCDB_RDONLY))
			dbflags = DB_WRITECURSOR;

	signal_crit_enter();
	*cookie = NULL;
	if (0 == (rc = dbp->cursor(dbp, NULL, &dbc, dbflags)))
			*cookie = (void *)dbc;
	signal_crit_exit();

	if (dbflags & DB_WRITECURSOR)
			dddbg("%s: read-write cursor: %p", __func__, dbc);
	else
			dddbg("%s: read-only cursor: %p", __func__, dbc);

	if (0 == rc)
			return;

	errx(EXIT_FAILURE, "fail: cursor: %s, %s", db_strerror(rc),
					dbfile);
}

int
db_query_lookup(void **cookie, struct db_query *query,
                struct nlpcdbrp *pair)
{
        struct nlpcdbr  *raw;
        int             rc;
        DBC             *dbc;
        DBT             key, data;

        assert(NULL != cookie);
        assert(NULL != pair);
        assert(NULL != query);

        if (0 == query->limit_scan) {
                dddbg("query limit exceeded");
                return(-1);
        }

        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        key.data = pair->hash;
        key.size = NLPCR_HASHLEN;

        signal_crit_enter();
        if (NULL == (dbc = *(DBC **)cookie)) {
                signal_crit_exit();
                dddbg("warn: cookie unset (lookup)");
                return(0);
        }

        rc = dbc->c_get(dbc, &key, &data, DB_SET);
        signal_crit_exit();

        if (0 != rc) {
                if (DB_NOTFOUND == rc)
                        return(0);
                errx(EXIT_FAILURE, "fail: lookup: %s, %s",
                                db_strerror(rc), dbfile);
        }

        raw = (struct nlpcdbr *)data.data;

        if (query->field & DB_FIELD_STATE)
                if ( ! query_state(raw, query->state))
                        return(0);
        if (query->field & DB_FIELD_AUTH)
                if ( ! query_auth(raw, &query->addr))
                        return(0);

        memset(&pair->record, 0, sizeof(struct nlpcdbr));
        copyout(raw, &pair->record);

        query->limit_scan--;

        return(1);
}


int
db_query_next(void **cookie, struct db_query *query,
                struct nlpcdbrp *pair)
{
        struct nlpcdbr  *raw;
        int             rc;
        DBC             *dbc;
        DBT             key, data;

        assert(NULL != cookie);
        assert(NULL != pair);
        assert(NULL != query);

        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        signal_crit_enter();
        if (NULL == (dbc = *(DBC **)cookie)) {
                signal_crit_exit();
                dddbg("warn: cookie unset (next)");
                return(0);
        }

        for (;;) {
                if (0 == query->limit_scan) {
                        dddbg("query limit exceeded");
                        signal_crit_exit();
                        return(-1);
                }
                if (0 != (rc = dbc->c_get(dbc, &key, &data, DB_NEXT)))
                        break;
                raw = (struct nlpcdbr *)data.data;

                /*
                 * Process the query fields.  At this time, the query is
                 * restricted to the "state" field, so we needn't have a
                 * a complex mechanism (yet).
                 */

                if (query->field & DB_FIELD_STATE)
                        if ( ! query_state(raw, query->state))
                                continue;
                if (query->field & DB_FIELD_AUTH)
                        if ( ! query_auth(raw, &query->addr))
                                continue;

                query->limit_scan--;

                /* Match found. */

                memset(pair, 0, sizeof(struct nlpcdbrp));
                copyout(raw, &pair->record);
                memcpy(pair->hash, (unsigned char *)key.data,
                                NLPCR_HASHLEN);
                signal_crit_exit();
                return(1);

        }
        signal_crit_exit();

        if (DB_NOTFOUND != rc)
                errx(EXIT_FAILURE, "fail: sequence: %s, %s",
                                db_strerror(rc), dbfile);
        return(0);
}


void
db_query_close(void **cookie)
{
        int             rc;
        DBC             *dbc;

        assert(NULL != cookie);

        signal_crit_enter();
        if (NULL == (dbc = *(DBC **)cookie)) {
                signal_crit_exit();
                return;
        }
        dddbg("%s: cursor: %p", __func__, dbc);
        rc = dbc->c_close(dbc);
        *cookie = NULL;
        signal_crit_exit();

        if (0 == rc)
                return;

        errx(EXIT_FAILURE, "fail: cursor close: %s, %s",
                        db_strerror(rc), dbfile);
}


void
db_query_commit(void **cookie, struct nlpcdbrp *pair)
{
        struct nlpcdbr  raw;
        int             rc;
        DBC             *dbc;
        DBT             key, data;

        assert(NULL != cookie);

        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        key.data = pair->hash;
        key.size = NLPCR_HASHLEN;

        copyin(&pair->record, &raw);
        data.data = &raw;
        data.size = sizeof(struct nlpcdbr);

        signal_crit_enter();
        if (NULL == (dbc = *(DBC **)cookie)) {
                signal_crit_exit();
                return;
        }
        rc = dbc->c_put(dbc, &key, &data, DB_CURRENT);
        signal_crit_exit();

        if (0 == rc)
                return;

        errx(EXIT_FAILURE, "fail: cursor put: %s, %s",
                        db_strerror(rc), dbfile);
}


void
db_query_del(void **cookie)
{
        DBC             *dbc;
        int             rc;

        assert(NULL != cookie);

        signal_crit_enter();
        if (NULL == (dbc = *(DBC **)cookie)) {
                signal_crit_exit();
                return;
        }
        rc = dbc->c_del(dbc, 0);
        signal_crit_exit();

        if (0 == rc)
                return;

        errx(EXIT_FAILURE, "fail: cursor delete: %s, %s",
                        db_strerror(rc), dbfile);
}
