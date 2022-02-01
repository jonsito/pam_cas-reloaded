#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "cas.h"
#include "config.h"

static void get_password(char *password) {

    static struct termios old_terminal;
    static struct termios new_terminal;

    //get settings of the actual terminal
    tcgetattr(STDIN_FILENO, &old_terminal);
    // do not echo the characters
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ECHO);
    // set this as the new terminal options
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
    // get the password
    // the user can add chars and delete if he puts it wrong
    // the input process is done when he hits the enter
    // the \n is stored, we replace it with \0
    if (fgets(password, BUFSIZ, stdin) == NULL)
        password[0] = '\0';
    else
        password[strlen(password)-1] = '\0';
    // go back to the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
}

int main(int argc, char **argv) {
	struct CAS cas;
	int ret = 0;
	CAS_configuration c;

    char username[BUFSIZ];
    char password[BUFSIZ];

    // get username and password from stdin
    fprintf(stdout,"Enter username: ");
    if (fgets(username, BUFSIZ, stdin) == NULL)  username[0] = '\0';
    else username[strlen(username)-1] = '\0';
    fprintf(stdout,"Enter password: ");
    get_password(password);
    if ( !username[0] || !password[0]) {
        fprintf(stderr,"Error: must provide username and password\n");
        exit(1);
    }
    fprintf(stdout,"\n");

    // load configuration
	ret = load_config(&c, PAM_CAS_CONFIGFILE);
	if (!ret) {
		printf("Failed to load configuration!\n");
		exit(1);
	}
	printf("Loaded config:\n");
	printf("BASE_URL = %s\n", c.CAS_BASE_URL);
	printf("SERVICE_URL = %s\n", c.SERVICE_URL);
	printf("Enable serviceValidate = %d\n", c.ENABLE_ST);
	printf("Enable proxyValidate = %d\n", c.ENABLE_PT);
	printf("Enable user+pass = %d\n", c.ENABLE_UP);

	// Initialize the CAS backend
	CAS_init(&cas, c.CAS_BASE_URL, c.SERVICE_URL, c.SERVICE_CALLBACK_URL);
	// Attempt a full login with user/pass
	ret = CAS_login(&cas, username, password);
	// Ret > 1 means we are authenticated 
	if (ret > 0)
		printf("We are now authenticated!\n");
	else
		printf("Failed to authenticate!\n");
	// Clean up resources
	CAS_cleanup(&cas);

	return 0;
}
