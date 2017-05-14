#include "meshquad.h"
#include "matrices.h"
#include <QDebug>

MeshQuad::MeshQuad():
	m_nb_ind_edges(0)
{

}


void MeshQuad::gl_init()
{
    m_shader_flat = new ShaderProgramFlat();
	m_shader_color = new ShaderProgramColor();

	//VBO
	glGenBuffers(1, &m_vbo);

	//VAO
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_flat->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_flat->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &m_vao2);
	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_color->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_color->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);


	//EBO indices
	glGenBuffers(1, &m_ebo);
	glGenBuffers(1, &m_ebo2);
}

void MeshQuad::gl_update()
{
    //VBO
    if (!m_points.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * m_points.size() * sizeof(GLfloat), &(m_points[0][0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    std::vector<int> tri_indices;
    convert_quads_to_tris(m_quad_indices,tri_indices);

    //EBO indices
    if (!tri_indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,tri_indices.size() * sizeof(int), &(tri_indices[0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    std::vector<int> edge_indices;
    convert_quads_to_edges(m_quad_indices,edge_indices);
    m_nb_ind_edges = edge_indices.size();

    if (m_nb_ind_edges > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo2);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_nb_ind_edges * sizeof(int), &(edge_indices[0]), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void MeshQuad::set_matrices(const Mat4& view, const Mat4& projection)
{
	viewMatrix = view;
	projectionMatrix = projection;
}

void MeshQuad::draw(const Vec3& color)
{

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);

	m_shader_flat->startUseProgram();
	m_shader_flat->sendViewMatrix(viewMatrix);
	m_shader_flat->sendProjectionMatrix(projectionMatrix);
	glUniform3fv(m_shader_flat->idOfColorUniform, 1, glm::value_ptr(color));
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo);
	glDrawElements(GL_TRIANGLES, 3*m_quad_indices.size()/2,GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	m_shader_flat->stopUseProgram();

	glDisable(GL_POLYGON_OFFSET_FILL);

	m_shader_color->startUseProgram();
	m_shader_color->sendViewMatrix(viewMatrix);
	m_shader_color->sendProjectionMatrix(projectionMatrix);
	glUniform3f(m_shader_color->idOfColorUniform, 0.0f,0.0f,0.0f);
	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo2);
	glDrawElements(GL_LINES, m_nb_ind_edges,GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	m_shader_color->stopUseProgram();
}

void MeshQuad::clear()
{
	m_points.clear();
	m_quad_indices.clear();
}

int MeshQuad::add_vertex(const Vec3& P)
{
    m_points.push_back(P);  // on ajoute le sommet en fin de liste
    return m_points.size() - 1; // on retourne l'indice du sommet inséré
}


void MeshQuad::add_quad(int i1, int i2, int i3, int i4)
{
    m_quad_indices.push_back(i1);
    m_quad_indices.push_back(i2);
    m_quad_indices.push_back(i3);
    m_quad_indices.push_back(i4);
}

void MeshQuad::convert_quads_to_tris(const std::vector<int>& quads, std::vector<int>& tris)
{
	tris.clear();
	tris.reserve(3*quads.size()/2); // 1 quad = 4 indices -> 2 tris = 6 indices d'ou ce calcul (attention division entiere)

	// Pour chaque quad on genere 2 triangles
	// Attention a respecter l'orientation des triangles
    for (int i = 0; i < quads.size(); i+=4)
    {
        int i1 = quads[i];
        int i2 = quads[i+1];
        int i3 = quads[i+2];
        int i4 = quads[i+3];
        // triangle i1 i2 i4
        tris.push_back(i1);
        tris.push_back(i2);
        tris.push_back(i4);
        // triangle i2 i3 i4
        tris.push_back(i2);
        tris.push_back(i3);
        tris.push_back(i4);
    }
}

void MeshQuad::convert_quads_to_edges(const std::vector<int>& quads, std::vector<int>& edges)
{
	edges.clear();
	edges.reserve(quads.size()); // ( *2 /2 !)

	// Pour chaque quad on genere 4 aretes, 1 arete = 2 indices.
	// Mais chaque arete est commune a 2 quads voisins !
	// Comment n'avoir qu'une seule fois chaque arete ?

    // recherche l'arrête i1 i2 jusqu'à l'indice j
    auto rechercheArrete = [&] (int i1, int i2, int j) -> bool
    {
        for (int k = 0; k < j; k++)
        {
            if (i1 == edges[k] && i2 == edges[k+1])
            {
                return true;    // déjà présente
            }
        }
        return false;
    };

    int insertions = 0; // nb d'insertions dans edges (taille courante)
    for (int i = 0; i < quads.size(); i+=4)
    {
        int i1 = quads[i];
        int i2 = quads[i+1];
        int i3 = quads[i+2];
        int i4 = quads[i+3];

        // ajout si non déjà ajouté précédemment
        if (!rechercheArrete(i1, i2, insertions))
        {
            edges.push_back(i1);
            edges.push_back(i2);
            insertions++;
        }

        if (!rechercheArrete(i2, i3, insertions))
        {
            edges.push_back(i2);
            edges.push_back(i3);
            insertions++;
        }

        if (!rechercheArrete(i3, i4, insertions))
        {
            edges.push_back(i3);
            edges.push_back(i4);
            insertions++;
        }

        if (!rechercheArrete(i4, i1, insertions))
        {
            edges.push_back(i4);
            edges.push_back(i1);
            insertions++;
        }
    }
}


void MeshQuad::create_cube()
{
	clear();

	// ajouter 8 sommets (-1 +1)
    int p1 = add_vertex(Vec3(0,0,0));
    int p2 = add_vertex(Vec3(1,0,0));
    int p3 = add_vertex(Vec3(1,1,0));
    int p4 = add_vertex(Vec3(0,1,0));
    int p5 = add_vertex(Vec3(1,0,1));
    int p6 = add_vertex(Vec3(0,0,1));
    int p7 = add_vertex(Vec3(0,1,1));
    int p8 = add_vertex(Vec3(1,1,1));

	// ajouter 6 faces (sens trigo)
    add_quad(p4,p3,p2,p1);
    add_quad(p1,p2,p5,p6);
    add_quad(p1,p6,p7,p4);
    add_quad(p6,p5,p8,p7);
    add_quad(p2,p3,p8,p5);
    add_quad(p3,p4,p7,p8);

	gl_update();
}

Vec3 MeshQuad::normal_of_quad(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
	// Attention a l'ordre des points !
	// le produit vectoriel n'est pas commutatif U ^ V = - V ^ U
	// ne pas oublier de normaliser le resultat.

    Vec3 AB(A - B);
    Vec3 AD(A - D);
    Vec3 BA(B - A);
    Vec3 BC(B - C);
    Vec3 CB(C - B);
    Vec3 CD(C - D);
    Vec3 DA(D - A);
    Vec3 DC(D - C);
    Vec3 normaleA = glm::normalize(glm::cross(AB,AD));
    Vec3 normaleB = glm::normalize(glm::cross(BC,BA));
    Vec3 normaleC = glm::normalize(glm::cross(CD,CB));
    Vec3 normaleD = glm::normalize(glm::cross(DA,DC));

    Vec3 normale((normaleA.x + normaleB.x + normaleC.x + normaleD.x)/4,
                 (normaleA.y + normaleB.y + normaleC.y + normaleD.y)/4,
                 (normaleA.z + normaleB.z + normaleC.z + normaleD.z)/4);
    qDebug() << "normale : " << normale.x << " | " << normale.y << " | " << normale.z;

    return normale;
}

float MeshQuad::area_of_quad(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
    // aire du quad - aire tri + aire tri
    // aire du tri = 1/2 aire parallelogramme
    // aire parallelogramme: cf produit vectoriel

    //aire de ABD
    Vec3 AB(B - A);
    Vec3 AD(D - A);

    float aireABD = glm::length(AB)*glm::length(AD)*glm::sin(glm::acos(glm::dot(AB,AD)))/2.0f;

    //aire de BCD
    Vec3 CB(B - C);
    Vec3 CD(D - C);

    float aireBCD = glm::length(CB)*glm::length(CD)*glm::sin(glm::acos(glm::dot(CB,CD)))/2.0f;
    qDebug() << "aire tri 1 : " << aireABD << " et aire tri 2 : " << aireBCD;

    return aireABD + aireBCD;
}

bool MeshQuad::is_points_in_quad(const Vec3& P, const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
    // On sait que P est dans le plan du quad.
    Vec3 norm_quad = normal_of_quad(A,B,C,D);
    Vec3 AB = Vec3(B.x-A.x,B.y-A.y,B.z-A.z);
    Vec3 BC = Vec3(C.x-B.x,C.y-B.y,C.z-B.z);
    Vec3 CD = Vec3(D.x-C.x,D.y-C.y,D.z-C.z);
    Vec3 DA = Vec3(A.x-D.x,A.y-D.y,A.z-D.z);

    Vec3 normaleABn = glm::normalize(glm::cross(norm_quad,AB));
    Vec3 normaleBCn = glm::normalize(glm::cross(norm_quad,BC));
    Vec3 normaleCDn = glm::normalize(glm::cross(norm_quad,CD));
    Vec3 normaleDAn = glm::normalize(glm::cross(norm_quad,DA));
    int d_ABn = normaleABn.x*A.x + normaleABn.y*A.y + normaleABn.z*A.z;
    int d_BCn = normaleBCn.x*B.x + normaleBCn.y*B.y + normaleBCn.z*B.z;
    int d_CDn = normaleCDn.x*C.x + normaleCDn.y*C.y + normaleCDn.z*C.z;
    int d_DAn = normaleDAn.x*D.x + normaleDAn.y*D.y + normaleDAn.z*D.z;

    int p_dessus_ABn = normaleABn.x*P.x + normaleABn.y*P.y + normaleABn.z*P.z - d_ABn;
    int p_dessus_BCn = normaleBCn.x*P.x + normaleBCn.y*P.y + normaleBCn.z*P.z - d_BCn;
    int p_dessus_CDn = normaleCDn.x*P.x + normaleCDn.y*P.y + normaleCDn.z*P.z - d_CDn;
    int p_dessus_DAn = normaleDAn.x*P.x + normaleDAn.y*P.y + normaleDAn.z*P.z - d_DAn;
    qDebug() << "p_dessus : " << p_dessus_ABn << " | " << p_dessus_BCn << " | " << p_dessus_CDn << " | " << p_dessus_DAn;

    // P est-il au dessus des 4 plans contenant chacun la normale au quad et une arete AB/BC/CD/DA ?
    // si oui il est dans le quad
    if( (p_dessus_ABn >= 0) && (p_dessus_BCn >= 0) && (p_dessus_CDn >= 0) && (p_dessus_DAn >= 0) )
    {
        return true;
    }
    else return false;
}

bool MeshQuad::intersect_ray_quad(const Vec3& P, const Vec3& Dir, int q, Vec3& inter)
{
	// recuperation des indices de points
	// recuperation des points   
    // calcul de l'equation du plan (N+d)
    // calcul de l'intersection rayon plan
    // I = P + alpha*Dir est dans le plan => calcul de alpha
    // alpha => calcul de I
    // I dans le quad ?

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3 A = m_points[indiceA];
    Vec3 B = m_points[indiceB];
    Vec3 C = m_points[indiceC];
    Vec3 D = m_points[indiceD];

    // récupération de la normale du quad
    Vec3 normale = normal_of_quad(A,B,C,D);

    // récupération du centre du quad
    Vec3 centre((A.x+B.x+C.x+D.x)/4.0f, (A.y+B.y+C.y+D.y)/4.0f, (A.z+B.z+C.z+D.z)/4.0f);

    // calcul du d
    int d = normale.x*centre.x + normale.y*centre.y + normale.z*centre.z;

    // calcul de alpha
    float alpha = -(glm::dot(P,normale) + d)/(glm::dot(Dir,normale));
    inter = Vec3(P + alpha*Dir);

    qDebug() << "alpha = " << alpha << " et donc inter = " << inter.x << "," << inter.y << "," << inter.z;

    if (is_points_in_quad(inter,A,B,C,D))
    {
        return true;
    }
    else
    {
        return false;
    }
}


int MeshQuad::intersected_visible(const Vec3& P, const Vec3& Dir)
{
	// on parcours tous les quads
	// on teste si il y a intersection avec le rayon
	// on garde le plus proche (de P)

    int inter = -1; // retour de la fonction
    float distance = -1;    // infini
    Vec3 ptInter(0,0,0);

    for (int i = 0; i < m_quad_indices.size()/4; i++)
    {
        // tester si P est dans le quad
        if (intersect_ray_quad(P, Dir, i, ptInter))
        {
            // il y a une intersection, on la conserve si elle est plus proche de P que la précédente
            if (distance < 0)
            {
                Vec3 ecart(ptInter - P);    // nouvelle distance P - inter
                float norme = vec_length(ecart);
                distance = norme;
                inter = i;  // nouvelle plus proche intersection
                qDebug() << "distance (<0) : " << distance;
            }
            else
            {
                Vec3 ecart(ptInter - P);    // nouvelle distance P - inter
                float norme = vec_length(ecart);

                if (norme > distance)
                {
                    distance = norme;
                    inter = i;  // nouvelle plus proche intersection
                    qDebug() << "distance (>=0) : " << distance;
                }
            }
        }
    }

    return inter;
}


Mat4 MeshQuad::local_frame(int q)
{
	// Repere locale = Matrice de transfo avec
	// les trois premieres colones: X,Y,Z locaux
	// la derniere colonne l'origine du repere
	// ici Z = N et X = AB
	// Origine le centre de la face
	// longueur des axes : [AB]/2
	// recuperation des indices de points
	// recuperation des points
	// calcul de Z:N puis de X:arete on en deduit Y
	// calcul du centre
	// calcul de la taille
	// calcul de la matrice

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3 A = m_points[indiceA];
    Vec3 B = m_points[indiceB];
    Vec3 C = m_points[indiceC];
    Vec3 D = m_points[indiceD];

    // origine = centre de la face
    Vec3 centre((A.x+B.x+C.x+D.x)/4.0f,(A.y+B.y+C.y+D.y)/4.0f,(A.z+B.z+C.z+D.z)/4.0f);

    // calcul de la composante X = AB (longueur ramenée à AB/2)
    Vec3 X = glm::normalize(Vec3(B - A));

    // calcul de la composante Z (normale)
    Vec3 Z = glm::normalize(normal_of_quad(A,B,C,D));

    // Y est orthogonal à X et Z
    Vec3 Y = glm::cross(X,Z);

    // matrice finale
    Mat4 local( X.x, X.y, X.z, 0.0f,
                Y.x, Y.y, Y.z, 0.0f,
                Z.x, Z.y, Z.z, 0.0f,
                centre.x, centre.y, centre.z, 1.0f);

    return local;
}

void MeshQuad::extrude_quad(int q)
{
	// recuperation des indices de points
	// recuperation des points
	// calcul de la normale
	// calcul de la hauteur
	// calcul et ajout des 4 nouveaux points
    // on remplace le quad initial par le quad du dessus
	// on ajoute les 4 quads des cotes

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3 A = m_points[indiceA];
    Vec3 B = m_points[indiceB];
    Vec3 C = m_points[indiceC];
    Vec3 D = m_points[indiceD];

    // aire du quad
    float aire = area_of_quad(A,B,C,D);

    // décalage de la racine carée de l'aire
    float decalage = (float)sqrt(aire);

    qDebug() << "aire du quad = " << aire << " et donc décalage = " << decalage;

    // récupération de la normale
    Vec3 normale = normal_of_quad(A,B,C,D);

    // calcul des 4 nouveaux points
    Vec3 nouveauA(A + normale*decalage);
    Vec3 nouveauB(B + normale*decalage);
    Vec3 nouveauC(C + normale*decalage);
    Vec3 nouveauD(D + normale*decalage);

    // ajout des 4 nouveaux points
    int indiceNouveauA = add_vertex(nouveauA);
    int indiceNouveauB = add_vertex(nouveauB);
    int indiceNouveauC = add_vertex(nouveauC);
    int indiceNouveauD = add_vertex(nouveauD);

    // remplacement du quad initial par le quad etrudé
    m_quad_indices[4*q] = indiceNouveauA;
    m_quad_indices[4*q+1] = indiceNouveauB;
    m_quad_indices[4*q+2] = indiceNouveauC;
    m_quad_indices[4*q+3] = indiceNouveauD;

    // et des 4 nouveaux quads formés
    add_quad(indiceA, indiceB, indiceNouveauB, indiceNouveauA);
    add_quad(indiceB, indiceC, indiceNouveauC, indiceNouveauB);
    add_quad(indiceC, indiceD, indiceNouveauD, indiceNouveauC);
    add_quad(indiceD, indiceA, indiceNouveauA, indiceNouveauD);

	gl_update();
}


void MeshQuad::decale_quad(int q, float d)
{
	// recuperation des indices de points
	// recuperation des (references de) points
	// calcul de la normale
	// modification des points

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3& A = m_points[indiceA];
    Vec3& B = m_points[indiceB];
    Vec3& C = m_points[indiceC];
    Vec3& D = m_points[indiceD];

    // calcul de la normale
    Vec3 normale = normal_of_quad(A,B,C,D);

    // calcul des 4 nouveaux points
    A += normale*d;
    B += normale*d;
    C += normale*d;
    D += normale*d;

	gl_update();
}


void MeshQuad::shrink_quad(int q, float s)
{
	// recuperation des indices de points
	// recuperation des (references de) points
    // ici pas besoin de passer par une matrice
	// calcul du centre
    // modification des points

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3& A = m_points[indiceA];
    Vec3& B = m_points[indiceB];
    Vec3& C = m_points[indiceC];
    Vec3& D = m_points[indiceD];

    // calcul du centre
    Vec3 centre((A.x+B.x+C.x+D.x)/4.0f,(A.y+B.y+C.y+D.y)/4.0f,(A.z+B.z+C.z+D.z)/4.0f);

    // calcul des vecteurs centre-point
    Vec3 centreA(A - centre);
    Vec3 centreB(B - centre);
    Vec3 centreC(C - centre);
    Vec3 centreD(D - centre);

    // calcul des 4 nouveaux points + modification
    A = centre + centreA*s;
    B = centre + centreB*s;
    C = centre + centreC*s;
    D = centre + centreD*s;

	gl_update();
}


void MeshQuad::tourne_quad(int q, float a)
{
	// recuperation des indices de points
	// recuperation des (references de) points
	// generation de la matrice de transfo:
	// tourne autour du Z de la local frame
	// indice utilisation de glm::inverse()
	// Application au 4 points du quad

    // récupération des indices du quad
    int indiceA = m_quad_indices[4*q];
    int indiceB = m_quad_indices[4*q + 1];
    int indiceC = m_quad_indices[4*q + 2];
    int indiceD = m_quad_indices[4*q + 3];

    // récupération des points du quad
    Vec3& A = m_points[indiceA];
    Vec3& B = m_points[indiceB];
    Vec3& C = m_points[indiceC];
    Vec3& D = m_points[indiceD];

    // construction de la matrice de rotation
    Mat4 loc = local_frame(q);
    glm::vec3 myRotationAxis(loc[2][0],loc[2][1],loc[2][2]);
    Mat4 rot = glm::rotate( a, myRotationAxis );

    // calcul des 4 nouveaux points + modification
    A = Vec3(Vec4(A,1.0f)*rot);
    B = Vec3(Vec4(B,1.0f)*rot);
    C = Vec3(Vec4(C,1.0f)*rot);
    D = Vec3(Vec4(D,1.0f)*rot);

	gl_update();
}

