#include "viewer.h"

#include <QKeyEvent>
#include <iomanip>


Viewer::Viewer():
	QGLViewer(),
	ROUGE(1,0,0),
	VERT(0,1,0),
	BLEU(0,0,1),
	JAUNE(1,1,0),
	CYAN(0,1,1),
	MAGENTA(1,0,1),
	BLANC(1,1,1),
	GRIS(0.5,0.5,0.5),
	NOIR(0,0,0),
    m_code(0)   // 1 = draw repère
{}


void Viewer::init()
{
	makeCurrent();
	glewExperimental = GL_TRUE;
	int glewErr = glewInit();
	if( glewErr != GLEW_OK )
	{
		qDebug("Error %s", glewGetErrorString(glewErr) ) ;
	}
	std::cout << "GL VERSION = " << glGetString(GL_VERSION) << std::endl;

	// la couleur de fond
	glClearColor(0.2,0.2,0.2,0.0);

	// QGLViewer initialisation de la scene
	setSceneRadius(9.0);
	setSceneCenter(qglviewer::Vec(0.0,0.0,0.0));
	camera()->showEntireScene();

	// initilisation des primitives
	m_prim.gl_init();

	// initialisation variables globales
	m_compteur = 0;
	m_angle1 = 0.0;
	m_angle2 = 0.0;
}



void Viewer::draw_repere(const Mat4& global)
{
	//	// exemple de definition de fonction (lambda) locale
	//	float b=2.2f;
	//	auto fonction_locale = [&] (float a)
	//	{
	//		std::cout << "param a="<< a << " & global b="<< b <<std::endl;
	//	};

	//	//appel
	//	fonction_locale(1.1f);

    auto fleche = [&] (Mat4 tr, Vec3 coul) -> void
    {
        m_prim.draw_cylinder(tr*translate(0,0,1.5)*scale(0.5,0.5,2.5), coul);
        m_prim.draw_cone(tr*translate(0,0,3), coul);
    };

    m_prim.draw_sphere(global, BLANC);
    fleche(global, BLEU);
    fleche(global*rotateY(90), ROUGE);
    fleche(global*rotateX(-90), VERT);

    // axe X
    //m_prim.draw_cylinder(rotateY(90)*translate(0,0,1.5)*scale(0.5,0.5,2.5), ROUGE);
    //m_prim.draw_cone(rotateY(90)*translate(0,0,3), ROUGE);
    // axe Y
    //m_prim.draw_cylinder(rotateX(-90)*translate(0,0,1.5)*scale(0.5,0.5,2.5), VERT);
    //m_prim.draw_cone(rotateX(-90)*translate(0,0,3), VERT);
    // axe Z
    //m_prim.draw_cylinder(translate(0,0,1.5)*scale(0.5,0.5,2.5), BLEU);
    //m_prim.draw_cone(translate(0,0,3), BLEU);
}



void Viewer::draw_main()
{
    // cvommencer par faire un doigt -> 3 phalanges (+ articulations)
    float a = m_compteur;

    auto doigt = [&] ( Mat4 trf ) -> void
    {
        m_prim.draw_sphere(trf, BLANC);    // articulation de base
        m_prim.draw_cube(trf*rotateZ(a)*translate(1.5,0,0)*scale(2,0.5,0.8), ROUGE);

        trf *= rotateZ(a)*translate(3,0,0);

        m_prim.draw_sphere(trf, BLANC);
        m_prim.draw_cube(trf*rotateZ(a)*translate(1.5,0,0)*scale(2.0,0.5,0.8), VERT);

        trf *= rotateZ(a)*translate(3,0,0);

        m_prim.draw_sphere(trf, BLANC);
        m_prim.draw_cube(trf*rotateZ(a)*translate(1.5,0,0)*scale(1.5,0.5,0.8), BLEU);
    };

    doigt(Mat4());
    doigt(translate(0,0,2));
    doigt(translate(0,0,4));

}

void Viewer::draw_basic()
{
	m_prim.draw_sphere(Mat4(), BLANC);
    m_prim.draw_cube(translate(3,0,0), ROUGE);
    m_prim.draw_cone(translate(0,3,0), VERT);
    m_prim.draw_cylinder(translate(0,0,3), BLEU);

}

void Viewer::draw()
{
	makeCurrent();
	m_prim.set_matrices(getCurrentModelViewMatrix(),getCurrentProjectionMatrix());

	Mat4 glob;

	switch(m_code)
	{
		case 0:
			draw_basic();
		break;
		case 1:
			draw_repere(glob);
		break;
		case 2:
			// coder ici les petits reperes qui tournet autour du grand repere
            draw_repere(glob);  // grand repère
            // le nombre de repères tounant  autour du grand
            #define NB 100
            for(int i = 0; i < NB; i++)
            {
                glob = rotateZ(10)*rotateY(-m_compteur-(i*360/NB))*translate(6,0,0)*rotateY(-90)*scale(0.5,0.5,0.5)*rotateX(5*m_compteur);
                draw_repere(glob);  // un petit repère
            }

		break;
        case 3:
            draw_main();
		break;
	}
}


void Viewer::keyPressEvent(QKeyEvent *e)
{

	if (e->modifiers() & Qt::ShiftModifier)
	{
		std::cout << "Shift is on"<< std::endl;
	}

	if (e->modifiers() & Qt::ControlModifier)
	{
		std::cout << "Control is on"<< std::endl;
	}

	switch(e->key())
	{
		case Qt::Key_Escape:
			exit(EXIT_SUCCESS);
			break;

		case Qt::Key_A: // touche 'a'
			if (animationIsStarted())
				stopAnimation();
			else
				startAnimation();
			break;

		case Qt::Key_M:  // change le code execute dans draw()
			m_code = (m_code+1)%4;
			break;
		default:
			break;
	}

	// retrace la fenetre
	updateGL();
	// passe la main a la QGLViewer
	QGLViewer::keyPressEvent(e);
}



void Viewer::animate()
{
	m_compteur += 1;

	// faire varier les angles ici pour animer
}




Mat4 Viewer::getCurrentModelViewMatrix() const
{
	GLdouble gl_mvm[16];
	camera()->getModelViewMatrix(gl_mvm);
	Mat4 mvm;
	for(unsigned int i = 0; i < 4; ++i)
	{
		for(unsigned int j = 0; j < 4; ++j)
			mvm[i][j] = float(gl_mvm[i*4+j]);
	}
	return mvm;
}


Mat4 Viewer::getCurrentProjectionMatrix() const
{
	GLdouble gl_pm[16];
	camera()->getProjectionMatrix(gl_pm);
	Mat4 pm;
	for(unsigned int i = 0; i < 4; ++i)
	{
		for(unsigned int j = 0; j < 4; ++j)
			pm[i][j] = float(gl_pm[i*4+j]);
	}
	return pm;
}
