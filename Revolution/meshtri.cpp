#include "meshtri.h"
#include "matrices.h"

MeshTri::MeshTri()
{
}



void MeshTri::gl_init()
{
	m_shader_flat = new ShaderProgramFlat();
	m_shader_phong = new ShaderProgramPhong();

	//VBO
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_vbo2);

	//VAO
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_flat->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_flat->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	//VAO2
	glGenVertexArrays(1, &m_vao2);
	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_phong->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_phong->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo2);
	glEnableVertexAttribArray(m_shader_phong->idOfNormalAttribute);
	glVertexAttribPointer(m_shader_phong->idOfNormalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	//EBO indices
	glGenBuffers(1, &m_ebo);
}

void MeshTri::gl_update()
{
    //VBO
    if (!m_points.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * m_points.size() * sizeof(GLfloat), &(m_points[0][0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!m_normals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo2);
        glBufferData(GL_ARRAY_BUFFER, 3 * m_normals.size() * sizeof(GLfloat), &(m_normals[0][0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //EBO indices
    if (!m_indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_indices.size() * sizeof(int), &(m_indices[0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}



void MeshTri::set_matrices(const Mat4& view, const Mat4& projection)
{
	viewMatrix = view;
	projectionMatrix = projection;
}


void MeshTri::draw(const Vec3& color)
{
	m_shader_flat->startUseProgram();

	m_shader_flat->sendViewMatrix(viewMatrix);
	m_shader_flat->sendProjectionMatrix(projectionMatrix);

	glUniform3fv(m_shader_flat->idOfColorUniform, 1, glm::value_ptr(color));

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo);
	glDrawElements(GL_TRIANGLES, m_indices.size(),GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	m_shader_flat->stopUseProgram();
}


void MeshTri::draw_smooth(const Vec3& color)
{
	m_shader_phong->startUseProgram();

	m_shader_phong->sendViewMatrix(viewMatrix);
	m_shader_phong->sendProjectionMatrix(projectionMatrix);

	glUniform3fv(m_shader_phong->idOfColorUniform, 1, glm::value_ptr(color));

	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo);
	glDrawElements(GL_TRIANGLES, m_indices.size(),GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);

	m_shader_phong->stopUseProgram();
}

// wipe out (sort of)
void MeshTri::clear()
{
    m_points.clear();
    m_normals.clear();
    m_indices.clear();
}

// ajoute un sommet au tableau de sommets et retourne son indice (size - 1 après ajout)
int MeshTri::add_vertex(const Vec3& P)
{
    m_points.push_back(P);
    return m_points.size() - 1;
}

// ajoute une normale au tableau des normales et retourne son indice (size - 1 après ajout)
int MeshTri::add_normal(const Vec3& N)
{
    m_normals.push_back(N);
    return m_normals.size() - 1;
}

void MeshTri::add_tri(int i1, int i2, int i3)
{
    m_indices.push_back(i1);
    m_indices.push_back(i2);
    m_indices.push_back(i3);
}

void MeshTri::add_quad(int i1, int i2, int i3, int i4)
{
	// decoupe le quad en 2 triangles: attention a l'ordre
    add_tri(i1,i2,i3);  // triangle 1
    add_tri(i1,i3,i4);  // triangle 2
}


void MeshTri::create_pyramide()
{
	clear();

    // 1) positions [-1;1]
    // a(-1,-1,0), b(1,-1,0), c(-1,1,0), d(1,1,0), e(0,0,1)
    // 2) topologie (faces)
    // 4 add_tri et 1 add_quad

    int a = add_vertex(Vec3(-1,-1,0));
    int b = add_vertex(Vec3(1,-1,0));
    int c = add_vertex(Vec3(1,1,0));
    int d = add_vertex(Vec3(-1,1,0));
    int e = add_vertex(Vec3(0,0,2));

    // on tourne dans le sens des aiguilles d'une montre car la base est derrière
    add_quad(a,d,c,b);

    // les faces tournent dans le sens trigonométrique car elles sont vues de face
    add_tri(a,b,e);
    add_tri(b,c,e);
    add_tri(c,d,e);
    add_tri(d,a,e);

	gl_update();
}

void MeshTri::create_anneau()
{
	clear();

    // boucle avec deux cercles

    int nb_quad = 100;   // nombre de carrés produits
    const float rayon_1 = 1.0f;
    const float rayon_2 = 1.5f;
    float alpha = 0;

    // ajout des points
    for (int i = 0; i < nb_quad; i++)
    {
        Vec3 P(rayon_1*std::cos(alpha), rayon_1*std::sin(alpha), 0.0);
        add_vertex(P);
        Vec3 Q(rayon_2*std::cos(alpha), rayon_2*std::sin(alpha), 0.0);
        add_vertex(Q);

        alpha += 2*M_PI/nb_quad;
    }

    // ajout des carrés
    for (int i = 0; i < nb_quad; i++)
    {
        add_quad(2*i, 2*i+2, 2*i+3, 2*i+1);
    }
    add_quad(2*nb_quad-2, 0, 1, 2*nb_quad-2+1);


	gl_update();
}

void MeshTri::create_spirale()
{
	clear();

    // boucle avec deux cercles + variation de z

    int nb_quad = 100;   // nombre de carrés produits
    int tours = 10;
    int n = nb_quad*tours;
    float rayon_1 = 2.0f;
    float alpha = 0.0f;
    float z = 0.0f;
    float h = 2.0f;

    // ajout des points
    for (int i = 0; i < n; i++)
    {
        Vec3 P(rayon_1*std::cos(alpha), rayon_1*std::sin(alpha), z);
        add_vertex(P);
        float rayon_2 = rayon_1-0.1f;
        Vec3 Q(rayon_2*std::cos(alpha), rayon_2*std::sin(alpha), z+h/(4*tours));
        add_vertex(Q);

        alpha += 2*M_PI/nb_quad;
        z += h/n;
        rayon_1 *= std::pow(0.1,1.0/n);
    }

    // ajout des carrés
    for (int i = 1; i < n; ++i)
    {
        add_quad(2*i-2, 2*i, 2*i+1, 2*i-1);
    }

	gl_update();
}


void MeshTri::revolution(const std::vector<Vec3>& poly)
{
	clear();

	// Faire varier angle 0 -> 360 par pas de D degre
	//   Faire tourner les sommets du polygon -> nouveau points

	// on obtient une grille de M (360/D x poly.nb) points

	// creation des quads qui relient ces points
	// attention la derniere rangee doit etre reliee a la premiere

	// on peut fermer le haut et le bas par ube ombrelle de triangles

    int n = poly.size();

    int m = 0;

    for (int alpha = 0; alpha < 360; alpha += 1)
    {
        Mat4 R = rotateY(alpha);
        // c++ 11 : "for range" for(var container)
        for (const Vec3& P: poly)
        {
            Vec3 Q = Vec3(R*Vec4(P,1));
            add_vertex(Q);
        }
        m++;
    }

    // les triangles
    for (int j = 0; j < m-1; ++j)
    {
        for (int i = 0; i < n-1; ++i)
        {
            int k = j*n + i;
            add_quad(k,k+1,k+1+n,k+n);
        }
    }

    // il faut encore relier la dernière et la première colonne
    for (int i = 0; i < n-1; ++i)
    {
        add_quad((m-1)*n+i, (m-1)*n+i+1, i+1, i);
    }

	gl_update();
}

void MeshTri::compute_normals()
{
	// ALGO:
	// init des normale a 0,0,0
	// Pour tous les triangles
	//   calcul de la normale -> N
	//   ajout de N au 3 normales des 3 sommets du triangles
	// Fin_pour
	// Pour toutes les normales
	//   normalisation
	// Fin_pour

}

