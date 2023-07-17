/*
 * alarm_cond.c
 *
 * This is an enhancement to the alarm_mutex.c program, which
 * used only a mutex to synchronize access to the shared alarm
 * list. This version adds a condition variable. The alarm
 * thread waits on this condition variable, with a timeout that
 * corresponds to the earliest timer request. If the main thread
 * enters an earlier timeout, it signals the condition variable
 * so that the alarm thread will wake up and process the earlier
 * timeout first, requeueing the later request.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64]; // message
    int                 mNum; //
    int                 messT;
    int                 justEntered;
    int                 changed;
    int                 nL;
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alarm_cond = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;
alarm_t *head, *tail;
int rCount;
char tempL[64]; // the deleted alarms message store temporary

/* Checks if the alarm is in.
 * First it checks weather the alarm a  message or a cancel amssage.
 * Then it goes through the alarm list, and checks if the alarm the caller
 * sends is there. 
 */
int alarmIsIn(alarm_t *alarm, int mType)
{
    alarm_t *next;
    int temp = 0;
    next = head->link;

    // Type Message
    if (mType == 1)
    {
 	// goes throught the list and finds massage number entered
    	while (next != tail)
    	{
	 	if (next->mNum == alarm->mNum && next-> messT == 1)
		{
		    	temp = 1;
		    	break;
		}
        	next = next->link;
    	}
    }
    // Type Cancel
    else if (mType == 0)
    {
	// // goes throught the cancel list and finds the message number entered
	while (next != tail)
    	{
                if (next->mNum == alarm->mNum && next->messT == 0)
                {
                    temp = 1;
                    break;
                }
        	next = next->link;
    	}
    }
    return temp;
}

/*
 * Replaces the current alarm with the new alarm entered.
 */
void replaceAlarm(alarm_t *alarm, int mType)
{
    alarm_t *next;

    if (mType == 0)
    {
	// goes throught the list untill alarm number is found similer to the replacing one
    	next = head->link;
    	while (next != tail)
    	{
		// type Message is need to replaced, so this groes through it
        	if (next->mNum == alarm->mNum && next->messT == 1)
		{
			// message content are replaced
			strcpy(next->message, alarm->message);
    	    		next->time = alarm->time;
    	    		next->seconds = alarm->seconds;
			next->changed = 1;
			break;
		}
        	next = next->link;
    	}
    }
}

/*
 * Inserts an alarm. 
 */
void alarm_insert (alarm_t *alarm)
{
    alarm_t *last, *next;
    int temp;

    // check if the alarm number is in the list already
    temp = alarmIsIn(alarm, 1);

    // if so replace the content of the alarm, in this case message
    if(temp)
    {
    	replaceAlarm(alarm, 0);
    }
    // if not go through the list and put alarm
    if (!temp)
    {
    	last = head;
	next = head->link;
	while (next != tail)
	{
		if (next->mNum >= alarm->mNum)
		{
			alarm->link = next;
		        last->link = alarm;
		        alarm->nL = 1;
		        break;
		}
		last = next;
		next = next->link;
	}

	if (next == tail)
	{
		alarm->link = next;
		last->link = alarm;
		alarm->nL = 1;
	}
     }
}

/*
 * Cancel messages entered ot the list.
 */
void alarm_cancel (alarm_t * alarm)
{
	alarm_t *last, * next;
	int temp = 0;
	int cancelTemp;

	// checks if the alarm need to delete is in
	temp = alarmIsIn(alarm, 1);

	// if not nothing happens, if it exist 
	if(temp)
	{
		// checks if the cancel list has the mesasge number is in or not
		cancelTemp = alarmIsIn(alarm, 0);
	
		// if not the cancel message is entered to the list
		if (!cancelTemp)
		{

			last = head;
			next = head->link;
			while (next != tail)
			{
				if (next->mNum >= alarm->mNum)
				{
					alarm->link = next;
					last->link = alarm;
					alarm->nL = 1;
					break;
				}
				last = next;
				next = next->link;
			}
		}
	}
}

/*
 * Locking setup.
 */
void lockingSetup ()
{
	alarm_t *alarm;
	int status;

	status = pthread_mutex_lock (&alarm_mutex);
	if (status != 0) err_abort (status, "Lock mutex");
        rCount++;

	if (rCount == 1)
        {
            status = pthread_mutex_lock (&alarm_cond);
	    if (status != 0) err_abort (status, "Lock mutex");
        }

	status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0) err_abort (status, "Unlock mutex");

}

/*
 * Unlocking setup.
 */
void unlockingSetup()
{
	alarm_t *alarm;
	int status;

	status = pthread_mutex_lock (&alarm_mutex);
        if (status != 0) err_abort (status, "Lock mutex");
        rCount--;
        
	if (rCount == 0)
        {
                status = pthread_mutex_unlock (&alarm_cond);
                if (status != 0) err_abort (status, "Unlock mutex");
        }
        
	status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0) err_abort (status, "Unlock mutex");
}

/*
 * Periodically dispaly the alarm massage according to the user alarm time input.
 * Furthermore, alarms are handled by own thread.Terminated when alarm is canceled.
 */
void *periodic_display_thread (void *arg)
{
    alarm_t *alarm;
    int status;
    int sleeptime;
    int temp = 0;

    int count = 0;
    alarm = arg;
    int lock = 1;
    int count2 = 0;

    while(1)
    {
	    lockingSetup ();
	    sleeptime = alarm->seconds;

	    // if message is remove, following message shows.
	    if(alarm->nL == 0)
	    {
		    printf("DISPLAY THREAD EXITING: Message(%d) %s\n", alarm->mNum, alarm->message);
		    unlockingSetup();
		    return NULL;
	    }

	    unlockingSetup();

	    // if the message is unchanged, the alarm goes through the list the users frequency time.
	    if (alarm->changed == 0 && count < alarm->seconds)
	    {
		    printf("Message(%d) %s\n", alarm->mNum, alarm->message);
		    count++;
	    }

	    // if the alarm is changed, following message is shown in display.
	    if (alarm->changed == 1 && temp == 0)
	    {
		    printf("MESSAGE CHANGED: Message (%d) %s\n", alarm->mNum, alarm->message);
		    temp = 1;
	    }

	    // if the alarm is changed, the alarm with the new message prints users entered amount of time.
	    if (alarm->changed == 1 && temp == 1 && count2 < alarm->seconds)
	    {
		    printf("Message (%d) %s\n", alarm->mNum, alarm->message);
		    temp = 1;
		    count2++;
	    }
		
	    unlockingSetup();
	    sleep(sleeptime);
    }
}

/*
 * Alarm thread. Searches for new alarms in the list periodically, if a new message type alarm 
 * is found then creates a periodic display thread, if a cancel message type found it removeos
 * the message need to be removed.
 *
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm, *next, *last;
    int status, deleteAlarm;
    pthread_t thread;
    char line[64];

    while (1) 
    {
		lockingSetup();

		// goes through the alarm list and finds the new alarm
		alarm = NULL;
	    	next = head->link;
		while (next != tail)
		{
		    if(next->justEntered == 1)
		    {
			next->justEntered = 0;
			alarm = next;
			break;
		    }
		    next = next->link;
		}

		if (alarm != NULL && alarm->messT == 1)
		{
		    	printf("DISPLAY THREAD CREATED FOR: Message (%d) %s\n ",alarm->mNum, alarm->message);
		    	status = pthread_create (&thread, NULL, periodic_display_thread, alarm);
    	    		if (status != 0) err_abort (status, "Create alarm thread");
		}

		unlockingSetup();

		// goes throught the cancel message list
		if (alarm != NULL &&  alarm->messT == 0)
		{
		    status = pthread_mutex_lock (&alarm_cond);
		    if (status != 0) err_abort (status, "Lock mutex");
		   
		    next = head->link;
		    last = head;
		    while (next != tail)
		    {
			if(next == alarm)
		        {
				deleteAlarm = next->mNum;
				last->link = next->link;
				next->nL = 0;
				break;
		        }
		    	last = next;
		    	next = next->link;
		    }

		    next = head->link;
		    last = head;
		    while (next != tail)
		    {
		        if(next->mNum == deleteAlarm)
		        {
				strcpy(line, next->message);
				next->nL = 0;
				last->link = next->link;
				break;
		        }
			last = next;
		        next = next->link;
		    }

		    // find the message to cancel, and cancels it.
		    printf("CANCEL: Message(%d) %s\n", alarm->mNum, tempL);
		    status = pthread_mutex_unlock (&alarm_cond);
		    if (status != 0) err_abort (status, "Lock mutex");		    
		}
    }
}

/*
 * The user promt. Read the user inputs, message types.Checks if the message is crrect.
 * Insertes the entered message to the according list.
 */
int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm;
    pthread_t thread;
    int flag;

    tail = (alarm_t*)malloc(sizeof(alarm_t));
    head = (alarm_t*)malloc(sizeof(alarm_t));
    tail->link = NULL;
    head->link = tail;
    rCount = 0;

    status = pthread_create (&thread, NULL, alarm_thread, NULL);
    if (status != 0) err_abort (status, "Create alarm thread");

    while (1)
    {
	flag = 1;
        printf ("Alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

	/*
         * Parse input line into seconds (%d), a message number (%d) and a
	 * message (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */
	if ((sscanf (line, "%d Message(%d) %64[^\n]",
            &alarm->seconds, &alarm->mNum, alarm->message) < 3) && (sscanf (line, "Cancel: Message(%d)", &alarm->mNum) < 1)) {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        }
	// Message Type
	else if (!(sscanf (line, "%d Message(%d) %64[^\n]",
		&alarm->seconds, &alarm->mNum, alarm->message) < 3))
	{
            status = pthread_mutex_lock (&alarm_cond);

	    if (status != 0) err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;
	    alarm->justEntered = 1;
	    alarm->changed = 0;
            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
	    alarm->messT = 1;
            alarm_insert (alarm);
            status = pthread_mutex_unlock (&alarm_cond);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
	// Cancel Type
	else if (!(sscanf (line, "Cancel: Message (%d)",  &alarm->mNum) < 1))
	{
	    alarm->seconds = 0;
	    strcpy(alarm->message, "Cancel commnad");
	    alarm->messT = 0;

            status = pthread_mutex_lock (&alarm_cond);
            if (status != 0) err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;
	    	alarm->justEntered = 1;
		alarm->changed = 0;
            /*
             * Insert the new alarm into the alarm list,
             * sorted by alarm number.
             */
	    alarm->messT = 0;
            alarm_cancel (alarm);
            status = pthread_mutex_unlock (&alarm_cond);
            if (status != 0) err_abort (status, "Unlock mutex");
	}
    }
}
