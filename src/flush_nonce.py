#!/usr/bin/python
import psycopg2

def execute(sql):
    """ Connect to the PostgreSQL database server """
    conn = None
    try:
        # connect to the PostgreSQL server
        print('Connecting to the PostgreSQL database...')        
        conn = psycopg2.connect(
            host="localhost",
            database="chirpstack_ns",
            user="chirpstack_ns",
            password="labsclabsc")
		
        # create a cursor
        cur = conn.cursor()
        
	    # execute a statement
        print('PostgreSQL database version:')
        cur.execute('SELECT version()')

        # display the PostgreSQL database server version
        db_version = cur.fetchone()
        print(db_version)

        # execute the UPDATE  statement
        cur.execute(sql)
        
        # Commit the changes to the database
        conn.commit()
        
	    # close the communication with the PostgreSQL
        cur.close()
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()
            print('Database connection closed.')


if __name__ == '__main__':
    sql = """ DELETE FROM public.device_activation  """
    
    execute(sql)
