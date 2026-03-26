#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//structuri pentru unitati, incidente si interventii
typedef struct unit {
	int id;
	char type;
	int availability;
	struct unit *next, *prev;
} Unit;

typedef struct incident {
	int id;
	char priority[7];
	char *description;
	char status[11];
	struct incident *next, *prev;
} Incident;

typedef struct intervention {
	struct incident *incident; 
	struct unit *unit;
	struct intervention *next, *prev;
} Intervention;

typedef struct system {
	struct unit *units;
	int units_count;
	struct incident *incidents;
	struct intervention *interventions;
} System;

//structuri pentru cozi si stive
typedef struct node {
	void *data;
	struct node *next;
} Node;

typedef struct queue {
	struct node *front, *rear;
	int size;
} Queue;

typedef struct stack {
	struct node *head;
	long size;
} Stack;

//functii de baza pentru cozi si stive
Node* create_node(void *data) {
	Node *new_node = (struct node*)malloc(sizeof(struct node));
	new_node->data = data;
	new_node->next = NULL;
	return new_node;
}

void enqueue(Queue *q, void *data) {
	Node *node;
	if (q->rear == NULL) {
		q->front = q->rear = create_node(data);
		q->size = 1;
		return;
	}
	node = create_node(data);
	q->rear->next = node;
	q->rear = node;
	q->size++;
}

//functia front adauga un element la inceputul cozii
void front(Queue *q, void *data) {
	Node *node = create_node(data);
	node->next = q->front;
	q->front = node;
	if (q->rear == NULL) {
		q->rear = node;
	}
	q->size++;
}

void* dequeue(Queue *q) {
	if (q->front != NULL) {
		Node *temp = q->front;
		void *data = temp->data;
		q->front = q->front->next;
		q->rear = (q->front == NULL) ? NULL : q->rear;
		free(temp);
		q->size--;
		return data;
	}
	return NULL;
}

void push(Stack *s, void *data) {
	Node *node = create_node(data);
	node->data = data;
	node->next = s->head;
	s->head = node;
	s->size++;
}

void* pop(Stack *s) {
	if (s->head != NULL) {
		Node *temp = s->head;
		void *data = temp->data;
		s->head = s->head->next;
		free(temp);
		s->size--;
		return data;
	}
	return NULL;
}

//santinela pentru incidente, care pointeaza catre ea insasi si e initializata cu incidentul de test
Incident* create_inc_sentinel() {
	Incident *sentinel = calloc(1, sizeof(Incident));
	sentinel->next = sentinel->prev = sentinel;
	sentinel->id = 0;
	sentinel->description = malloc(strlen("test incident") + 1);
	strcpy(sentinel->priority, "low");
	strcpy(sentinel->description, "test incident");
	strcpy(sentinel->status, "solved");
	return sentinel;;
}

//santinela pentru interventii care pointeaza catre ea insasi si e initializata cu NULL
Intervention* create_int_sentinel() {
	Intervention *sentinel = calloc(1, sizeof(Intervention));
	sentinel->next = sentinel->prev = sentinel;
	sentinel->incident = NULL;
	sentinel->unit = NULL;
	return sentinel;
}

//functie care adauga un incident la finalul listei de incidente,
//dar inaintea santinelei si il adauga in coada de prioritati corespunzatoare
//am ales sa am santinela la finalul listei pentru a fi mai usor de adaugat noile
//incidente la finalul listei
void add_incident(Incident *sentinel, Incident *node, Queue *qhigh, Queue *qmedium, Queue *qlow) {
	node->next = sentinel;
	node->prev = sentinel->prev;
	sentinel->prev->next = node;
	sentinel->prev = node;
	//am refacut legaturile vezi schema din caiet!!!
	strcpy(node->status, "queued");
	if (strcmp(node->priority, "high") == 0) {
		enqueue(qhigh, node);
	} else if (strcmp(node->priority, "medium") == 0) {
		enqueue(qmedium, node);
	} else {
		enqueue(qlow, node);
	}
}

//functie care adauga o interventie
void add_intervention(Intervention *sentinel, Intervention *node) {
	node->next = sentinel;
	node->prev = sentinel->prev;
	sentinel->prev->next = node;
	sentinel->prev = node;
}

void check_units_availability(Queue *qunits, FILE *out) {
	fprintf(out, "Number of available units: %d\n", qunits->size);
}

void dispatch(System *system, Queue *qhigh, Queue *qmedium, Queue *qlow, Queue *qunits, Stack *history, FILE *out) {
	Queue *target;
	//verific daca am incidente in coada de prioritati, in ordinea prioritatii, daca nu am niciun incident, eroare
	if (qhigh->size > 0) {
		target = qhigh;
	} else if (qmedium->size > 0) {
		target = qmedium;
	} else if (qlow->size > 0) {
		target = qlow;
	} else {
		fprintf(out, "INVALID OPERATION! ERROR 404\n");
		return;
	}
	//verific daca am unitati disponibile, daca nu am niciuna, eroare
	if (qunits->size == 0) {
		fprintf(out, "INVALID OPERATION! ERROR 404\n");
		return;
	}

	//am extras primul incident din coada de prioritati si prima unitate disponibila
	Incident *incident = (Incident*)dequeue(target);
	Unit *unit = (Unit*)dequeue(qunits);

	//am modificat statusul incidentului si valabilitatea unitatii folosite pentru acel incident
	strcpy(incident->status, "intervened");
	unit->availability = 0;

	//am creat o noua interventie si am adaugat o la finalul listei de interventii, dar inainte de santinela
	Intervention *intervention = (Intervention*)malloc(sizeof(Intervention));
	intervention->incident = incident;
	intervention->unit = unit;

	add_intervention(system->interventions, intervention);
	push(history, intervention); //ca sa pot sa tin minte la ce trebuie sa dau undo
}

void undo_last_dispatch(Queue *qhigh, Queue *qmedium, Queue *qlow, Queue *qunits, Stack *history, FILE *out) {
	//ultima interventie aflata in desfasurare care trebuie eliminata din lista de interventii
	//se cauta prima interventie care nu a fost inca finalizata, in istoricul interventiilor

	Intervention *intervention = NULL;
	while (history->head) {
		Intervention *temp = (Intervention*)pop(history);
		//am extras ultima interventie din istoric, acum verific daca a fost rezolvata sau nu
		if (strcmp(temp->incident->status, "intervened") == 0) {
			intervention = temp;
			break;
		}
	}

	//daca n am gasit nimic, eroare
	if (intervention == NULL) {
		fprintf(out, "INVALID OPERATION! ERROR 404\n");
		return;
	}

	//acum pur si simplu dau undo la status, adica din intervenit dau iar in queued
	strcpy(intervention->incident->status, "queued");
	//unitatea devine iar disponibila
	intervention->unit->availability = 1;
	//o pun in coada cu prioritatea ei
	if (strcmp(intervention->incident->priority, "high") == 0) {
		front(qhigh, intervention->incident);
	} else if (strcmp(intervention->incident->priority, "medium") == 0) {
		front(qmedium, intervention->incident);
	} else {
		front(qlow, intervention->incident);
	}

	//bag si unitatea in coada de unitati disponibile
	enqueue(qunits, intervention->unit);

	//scot interventia din lista
	intervention->prev->next = intervention->next;
	intervention->next->prev = intervention->prev;
	free(intervention);
}

void solved_incident(System *system, int id, Queue *qunits, FILE *out) {
	//trebuie sa ma plimb prin tot sistemul de incidente pana ajung la incidentul cu id ul dat
	//deci imi iau un incidnent curent
	Incident *current_incident = system->incidents->next;

	//ma plimb prin sistem
	while(current_incident != system->incidents) {
		if (current_incident->id == id) {
			//am gasit incidentul cu id ul dat
			//acum verific daca a fost intervenit sau nu
			if (strcmp(current_incident->status, "intervened")) {
				break;
				//nu a fost intervenit, ma opresc
			}
			//daca a fost intervenit
			strcpy(current_incident->status, "solved");
			//caut interventia corespunzatoare ca sa marchez unitatea ca disponibila
			Intervention *current_intervention = system->interventions->next;
			while(current_intervention != system->interventions) {
				if (current_intervention->incident == current_incident) {
					current_intervention->unit->availability = 1;
					//bag unitatea inapoi in coada de unitati disponibile
					enqueue(qunits, current_intervention->unit);
					return;
				}
				current_intervention = current_intervention->next;
			}
		}
		current_incident = current_incident->next;
	}

	//daca n am gasit nimic, eroare
	printf("INVALID OPERATION! ERROR 404\n");
	fprintf(out, "INVALID OPERATION! ERROR 404\n");
}

void show_unit(System *system, int id, FILE *out) {
	Unit *current_unit = system->units->next;
	//ma plimb prin sistem pana gasesc unitatea cu id ul dat
	while (current_unit != system->units) {
		if (current_unit->id == id) {
			fprintf(out, "Unit %d is type %c and is %s\n", id, current_unit->type,
				   current_unit->availability ? "available" : "unavailable");
			return;
		}
		current_unit = current_unit->next;
	}
	fprintf(out, "INVALID OPERATION! ERROR 404\n");
}

void show_incident(System *system, int id, FILE *out) {
	Incident *current_incident = system->incidents->next;
	//ma plimb prin sistem pana gasesc incidentul cu id ul dat
	while (current_incident != system->incidents) {
		if (current_incident->id == id) {
			fprintf(out, "Incident %d has %s priority, the following description: %s and is %s\n",
				   id, current_incident->priority, current_incident->description, current_incident->status);
			return;
		}
		current_incident = current_incident->next;
	}
	fprintf(out, "INVALID OPERATION! ERROR 404\n");
}

void show_interventions(System *system, FILE *out) {
	Intervention *current_intervention = system->interventions->next;
	//daca prima interventie e chiar santinela, inseamna ca nu am nicio interventie in desfasurare
	if (current_intervention == system->interventions) {
		fprintf(out, "No intervention has been initiated\n");
		return;
	}
	//ma plimb prin sistem si afisez toate interventiile
	while (current_intervention != system->interventions) {
		fprintf(out, "Incident %d was assigned to unit %d, and has the following status: \"%s\"\n",
			   current_intervention->incident->id, current_intervention->unit->id, current_intervention->incident->status);
		current_intervention = current_intervention->next;
	}
}

//functie care elibereaza toate resursele alocate din sistem, cozi si stive
void cleanup_system(System *system, Queue *qhigh, Queue *qmedium,
				   Queue *qlow, Queue *qunits, Stack *history) {
	//eliberare cozi
	while (dequeue(qhigh));
	while (dequeue(qmedium));
	while (dequeue(qlow));
	while (dequeue(qunits));
	
	//eliberare stiva, istoric
	Intervention *stacked_intervention;
	while ((stacked_intervention = (Intervention*)pop(history)) != NULL) {
		//eliberare interventie din lista circulara daca mai este prezenta
		stacked_intervention->prev->next = stacked_intervention->next;
		stacked_intervention->next->prev = stacked_intervention->prev;
		free(stacked_intervention);
	}

	//eliberare lista circulara de interventii
	Intervention *curr_int = system->interventions->next;
	while (curr_int != system->interventions) {
		Intervention *next = curr_int->next;
		free(curr_int); //incidentul si unitatea sunt eliberate in alte functii
		curr_int = next;
	}
	free(system->interventions); //eliberare santinela interventii

	//eliberare lista circulara de incidente
	Incident *curr_incident = system->incidents->next;
	while (curr_incident != system->incidents) {
		Incident *next = curr_incident->next;
		free(curr_incident->description); //eliberare string alocat dinamic
		free(curr_incident);
		curr_incident = next;
	}
	//eliberare santinela incidente
	free(system->incidents->description);
	free(system->incidents);

	//eliberare unitati
	if (system->units != NULL) free(system->units);
}

int main() {
	FILE *in = fopen("tema1.in", "r");
	FILE *out = fopen("tema1.out", "w");
	
	if (in == NULL || out == NULL) {
		printf("Error opening files\n");
		return 1;
	}

	System system;
	system.units = NULL;
	system.units_count = 0;
	system.incidents = create_inc_sentinel();
	system.interventions = create_int_sentinel();
	Queue qhigh = {NULL, NULL, 0};
	Queue qmedium = {NULL, NULL, 0};
	Queue qlow = {NULL, NULL, 0};
	Queue qunits = {NULL, NULL, 0};
	Stack history = {NULL, 0};

	int n_units = 0;
	fscanf(in, "%d", &n_units);
	system.units = (Unit*)calloc(1, sizeof(Unit));
	system.units->next = system.units->prev = system.units;
	system.units->id = 0;
	for (int i = 0; i < n_units; i++) {
		Unit *new_unit = (Unit*)malloc(sizeof(Unit));
		fscanf(in, "%d %c", &new_unit->id, &new_unit->type);
		new_unit->availability = 1;

		new_unit->next = system.units;
		new_unit->prev = system.units->prev;
		system.units->prev->next = new_unit;
		system.units->prev = new_unit;

		enqueue(&qunits, new_unit);
	}
	int n = 0;
	fscanf(in, "%d", &n);
	for (int i = 0; i < n; i++) {
		char command[30];
		fscanf(in, "%s", command);
		if (strcmp(command, "ADD_INCIDENT") == 0) {
			Incident *incident = (Incident*)malloc(sizeof(Incident));
			fscanf(in, "%d %s ", &incident->id, incident->priority);
			char buffer[256];
			fgets(buffer, sizeof(buffer), in);
			buffer[strcspn(buffer, "\n")] = 0;
			incident->description = strdup(buffer);
			add_incident(system.incidents, incident, &qhigh, &qmedium, &qlow);
		} else if (strcmp(command, "CHECK_UNITS_AVAILABILITY") == 0) {
			check_units_availability(&qunits, out);
		} else if (strcmp(command, "DISPATCH") == 0) {
			dispatch(&system, &qhigh, &qmedium, &qlow, &qunits, &history, out);
		} else if (strcmp(command, "UNDO_LAST_DISPATCH") == 0) {
			undo_last_dispatch(&qhigh, &qmedium, &qlow, &qunits, &history, out);
		} else if (strcmp(command, "SOLVED_INCIDENT") == 0) {
			int id;
			fscanf(in, "%d", &id);
			solved_incident(&system, id, &qunits, out);
		} else if (strcmp(command, "SHOW_UNIT") == 0) {
			int id;
			fscanf(in, "%d", &id);
			show_unit(&system, id, out);
		} else if (strcmp(command, "SHOW_INCIDENT") == 0) {
			int id;
			fscanf(in, "%d", &id);
			show_incident(&system, id, out);
		} else if (strcmp(command, "SHOW_INTERVENTIONS") == 0) {
			show_interventions(&system, out);
		}
	}
	cleanup_system(&system, &qhigh, &qmedium, &qlow, &qunits, &history);
	fclose(in);
	fclose(out);
	return 0;
}