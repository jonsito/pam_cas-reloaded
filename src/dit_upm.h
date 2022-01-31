//
// Created by jantonio on 30/1/22.
//

#ifndef PAM_CAS_RELOADED_DIT_UPM_H
#define PAM_CAS_RELOADED_DIT_UPM_H

#ifndef __DIT_UPM_C__
#define EXTERN extern
#else
#define EXTERN
#endif
EXTERN int ditupm_check(struct string *data);
EXTERN int eval_receivedCASData(struct string *data);
EXTERN int ditupm_generateLoginTicket(char *user, char *lt, size_t size);

#undef EXTERN
#endif //PAM_CAS_RELOADED_DIT_UPM_H
