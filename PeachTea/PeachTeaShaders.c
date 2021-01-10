#include "PeachTeaShaders.h"
#include "glExtensions.h"
#include "assetLoader.h"

float DEFAULT_QUAD_CORNERS[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
};

void PT_SHADERS_init() {
	qVAO = calloc(1, sizeof(GLuint));
	qVBO = calloc(2, sizeof(GLuint));

	glGenVertexArrays(1, qVAO);
	glBindVertexArray(*qVAO);
	glGenBuffers(2, qVBO);

	int quadPositions[] = {
		0, 0,
		0, 10,
		10, 10,
		10, 0
	};

	glBindBuffer(GL_ARRAY_BUFFER, *qVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadPositions), quadPositions, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, *(qVBO + 1));
	glBufferData(GL_ARRAY_BUFFER, sizeof(DEFAULT_QUAD_CORNERS), DEFAULT_QUAD_CORNERS, GL_STATIC_DRAW);

	GLuint qvs = create_vertex_shader("shaders\\core\\basicQuad.vs");

	GLuint tqfs = create_fragment_shader("shaders\\core\\textQuad.fs");
	GLuint ifs = create_fragment_shader("shaders\\core\\imageQuad.fs");
	GLuint uifs = create_fragment_shader("shaders\\core\\UIframe.fs");
	GLuint bfs = create_fragment_shader("shaders\\core\\blur.fs");
	GLuint tfs = create_fragment_shader("shaders\\core\\tex.fs");
	GLuint rfs = create_fragment_shader("shaders\\core\\rectangle.fs");

	GLuint quadShaders[] = { qvs, tqfs };
	PTS_text = create_program(quadShaders, 2);

	quadShaders[1] = ifs;
	PTS_img = create_program(quadShaders, 2);

	quadShaders[1] = uifs;
	PTS_guiObj = create_program(quadShaders, 2);

	quadShaders[1] = bfs;
	PTS_blur = create_program(quadShaders, 2);

	quadShaders[1] = tfs;
	PTS_tex = create_program(quadShaders, 2);

	quadShaders[1] = rfs;
	PTS_rect = create_program(quadShaders, 2);
}