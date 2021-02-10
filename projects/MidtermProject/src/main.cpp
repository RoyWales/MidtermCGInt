//Just a simple handler for simple initialization stuffs
#include "BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>



int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	

	bool opt1 = false; //options for lighting
	bool opt2 = false;
	bool opt3 = false;
	bool opt4 = true;
	bool opt5 = false;

	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link(); 

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 2.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 0.45f;
		float     lightSpecularPow = 2.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);  
		float     ambientPow = 0.4f; 
		float     lightLinearFalloff = 0.06f; 
		float     lightQuadraticFalloff = 0.02f;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

	/*	//new shader for height map
		Shader::sptr HeightShader = Shader::Create();
		HeightShader->LoadShaderPartFromFile("shaders/vertex_shaderHeight.glsl", GL_VERTEX_SHADER);
		HeightShader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		HeightShader->Link();

		HeightShader->SetUniform("u_LightPos", lightPos);
		HeightShader->SetUniform("u_LightCol", lightCol);
		HeightShader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		HeightShader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		HeightShader->SetUniform("u_AmbientCol", ambientCol);
		HeightShader->SetUniform("u_AmbientStrength", ambientPow);
		HeightShader->SetUniform("u_LightAttenuationConstant", 1.0f);
		HeightShader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		HeightShader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);*/

		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}
			shader->SetUniform("u_opt1", (int)opt1); //options for lighting
			shader->SetUniform("u_opt2", (int)opt2);
			shader->SetUniform("u_opt3", (int)opt3);
			shader->SetUniform("u_opt4", (int)opt4);
			shader->SetUniform("u_opt5", (int)opt5);
			//HeightShader->SetUniform("u_opt1", (int)opt1); //options for lighting
			//HeightShader->SetUniform("u_opt2", (int)opt2);
			//HeightShader->SetUniform("u_opt3", (int)opt3);
			//HeightShader->SetUniform("u_opt4", (int)opt4);
			//HeightShader->SetUniform("u_opt5", (int)opt5);

			if (ImGui::Checkbox("No Lighting", &opt1))
			{
				opt2 = false; 
				opt3 = false; 
				opt4 = false;
				opt5 = false;
			}
			if (ImGui::Checkbox("Ambient Only", &opt2))
			{
				opt1 = false;
				opt3 = false;
				opt4 = false;
				opt5 = false;     
			}
			if (ImGui::Checkbox("Specular Only", &opt3))
			{
				opt2 = false;
				opt1 = false;
				opt4 = false;
				opt5 = false;
			}
			if (ImGui::Checkbox("Specular, Ambient, and Diffuse", &opt4))
			{
				opt2 = false;
				opt3 = false;
				opt1 = false;
				opt5 = false;
			} 
			if (ImGui::Checkbox("Specular, Ambient, Diffuse and OutLine", &opt5))
			{ 
				opt2 = false;   
				opt3 = false;
				opt4 = false;
				opt1 = false;   
			} 

			shader->SetUniform("u_opt1", (int)opt1); 
			shader->SetUniform("u_opt2", (int)opt2);
			shader->SetUniform("u_opt3", (int)opt3);
			shader->SetUniform("u_opt4", (int)opt4);
			shader->SetUniform("u_opt5", (int)opt5);
			/*HeightShader->SetUniform("u_opt1", (int)opt1); //options for lighting
			HeightShader->SetUniform("u_opt2", (int)opt2);
			HeightShader->SetUniform("u_opt3", (int)opt3);
			HeightShader->SetUniform("u_opt4", (int)opt4);
			HeightShader->SetUniform("u_opt5", (int)opt5);*/
/*
			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);*/

			ImGui::Text("Q/E -> Up/Down\nA/D -> Left/Right\n");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr Car2 = Texture2D::LoadFromFile("images/Car2-Blue.png");
		Texture2D::sptr Tree = Texture2D::LoadFromFile("images/Tree.png");
		Texture2D::sptr Rock = Texture2D::LoadFromFile("images/Rock.png");
		Texture2D::sptr Railing = Texture2D::LoadFromFile("images/Railing.png");
		Texture2D::sptr Grass = Texture2D::LoadFromFile("images/sand.jpg");
		//Texture2D::sptr Road = Texture2D::LoadFromFile("images/Road.png");

		Texture2D::sptr diffuse2 = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr specular = Texture2D::LoadFromFile("images/White.png");
		Texture2D::sptr reflectivity = Texture2D::LoadFromFile("images/box-reflections.bmp");

		Texture2D::sptr HeightMap = Texture2D::LoadFromFile("images/HeightMap.png");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ocean.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		//materials
		// Create a material and set some properties for it
		ShaderMaterial::sptr CarMaterial = ShaderMaterial::Create();  
		CarMaterial->Shader = shader;
		CarMaterial->Set("s_Diffuse", Car2);
		CarMaterial->Set("s_Specular", specular);
		CarMaterial->Set("u_Shininess", 8.0f);
		CarMaterial->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr RockMaterial = ShaderMaterial::Create();
		RockMaterial->Shader = shader;
		RockMaterial->Set("s_Diffuse", Rock);
		RockMaterial->Set("s_Specular", specular);
		RockMaterial->Set("u_Shininess", 8.0f);
		RockMaterial->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr TreeMaterial = ShaderMaterial::Create();
		TreeMaterial->Shader = shader;
		TreeMaterial->Set("s_Diffuse", Tree);
		TreeMaterial->Set("s_Specular", specular);
		TreeMaterial->Set("u_Shininess", 8.0f);
		TreeMaterial->Set("u_TextureMix", 0.5f);

		ShaderMaterial::sptr RailingMaterial = ShaderMaterial::Create();
		RailingMaterial->Shader = shader;
		RailingMaterial->Set("s_Diffuse", Rock);
		RailingMaterial->Set("s_Specular", specular);
		RailingMaterial->Set("u_Shininess", 8.0f);
		RailingMaterial->Set("u_TextureMix", 0.5f);
		
		ShaderMaterial::sptr GrassMaterial = ShaderMaterial::Create();
		GrassMaterial->Shader = shader;
		GrassMaterial->Set("s_Diffuse", Grass);
		GrassMaterial->Set("s_Specular", specular);
		GrassMaterial->Set("u_Shininess", 8.0f);
		GrassMaterial->Set("u_TextureMix", 0.5f);
		GrassMaterial->Set("s_HeightMap", HeightMap);    


		// 
		/*ShaderMaterial::sptr material1 = ShaderMaterial::Create(); 
		material1->Shader = reflective;
		material1->Set("s_Diffuse", Car2);/////////////////////////////
		material1->Set("s_Diffuse2", diffuse2);
		material1->Set("s_Specular", specular);
		material1->Set("s_Reflectivity", reflectivity); 
		material1->Set("s_Environment", environmentMap); 
		material1->Set("u_LightPos", lightPos);
		material1->Set("u_LightCol", lightCol);
		material1->Set("u_AmbientLightStrength", lightAmbientPow); 
		material1->Set("u_SpecularLightStrength", lightSpecularPow); 
		material1->Set("u_AmbientCol", ambientCol);
		material1->Set("u_AmbientStrength", ambientPow);
		material1->Set("u_LightAttenuationConstant", 1.0f);
		material1->Set("u_LightAttenuationLinear", lightLinearFalloff);
		material1->Set("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		material1->Set("u_Shininess", 8.0f);
		material1->Set("u_TextureMix", 0.5f);
		material1->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
		
		ShaderMaterial::sptr reflectiveMat = ShaderMaterial::Create();
		reflectiveMat->Shader = reflectiveShader;
		reflectiveMat->Set("s_Environment", environmentMap);
		reflectiveMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));*/

		GameObject obj0 = scene->CreateEntity("Beach");
		{

			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Plane.obj");
			obj0.emplace<RendererComponent>().SetMesh(vao).SetMaterial(GrassMaterial);
			obj0.get<Transform>().SetLocalPosition(0.f, 0.0f, 0.0f).SetLocalScale(20, 15, 1); ///////////////////////////////////////////
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj0); 
		}

		/*GameObject obj2 = scene->CreateEntity("Chair");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chair1.obj");
			//VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chair1.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CarMaterial); //change mat
			obj2.get<Transform>().SetLocalPosition(0.0f, -2.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 180)).SetLocalScale(glm::vec3(1.5, 1.5, 1.5));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		GameObject obj10 = scene->CreateEntity("Chair");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chair1.obj");
			//VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chair1.obj");
			obj10.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CarMaterial); //change mat
			obj10.get<Transform>().SetLocalPosition(4.0f, -2.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 180)).SetLocalScale(glm::vec3(1.5, 1.5, 1.5));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj10);
		}*/

		GameObject obj3 = scene->CreateEntity("SurfBoard");
		{

			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/SurfBoard1.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(TreeMaterial);
			obj3.get<Transform>().SetLocalPosition(-6.0f, -6.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 90));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj3);
		}

		GameObject obj6 = scene->CreateEntity("SurfBoard");
		{

			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/SurfBoard1.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(TreeMaterial);
			obj6.get<Transform>().SetLocalPosition(6.0f, -8.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 0));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj6);
		}

		GameObject obj7 = scene->CreateEntity("SurfBoard");
		{

			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/SurfBoard1.obj");
			obj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(TreeMaterial);
			obj7.get<Transform>().SetLocalPosition(7.0f, -7.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 0));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj7);
		}

		GameObject obj4 = scene->CreateEntity("Cooler");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cooler1.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(RockMaterial);
			obj4.get<Transform>().SetLocalPosition(-3.0f, -2.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 90)).SetLocalScale(glm::vec3(0.7, 0.7, 0.7));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj4);
		}

		GameObject obj9 = scene->CreateEntity("Cooler");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cooler1.obj");
			obj9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(RockMaterial);
			obj9.get<Transform>().SetLocalPosition(7.0f, -2.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 90)).SetLocalScale(glm::vec3(0.7, 0.7, 0.7));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj9);
		}

		GameObject obj5 = scene->CreateEntity("Umbrella");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Umbrella1.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(RockMaterial);
			obj5.get<Transform>().SetLocalPosition(-2.0f, -1.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 0));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);
		}

		GameObject obj8 = scene->CreateEntity("Umbrella");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Umbrella1.obj");
			obj8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(RockMaterial);
			obj8.get<Transform>().SetLocalPosition(6.0f, -1.0f, 0.0f).SetLocalRotation(glm::vec3(90, 0, 0));
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj8);
		}

		

		/*
		GameObject obj4 = scene->CreateEntity("moving_box");
		{
			// Build a mesh
			MeshBuilder<VertexPosNormTexCol> builder = MeshBuilder<VertexPosNormTexCol>();
			MeshFactory::AddCube(builder, glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f));
			VertexArrayObject::sptr vao = builder.Bake();
			
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CarMaterial);
			obj4.get<Transform>().SetLocalPosition(-2.0f, 0.0f, 1.0f);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj4);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -4.0f, -4.0f, 0.0f });
			pathing->Points.push_back({ 4.0f, -4.0f, 0.0f });
			pathing->Points.push_back({ 4.0f,  4.0f, 0.0f });
			pathing->Points.push_back({ -4.0f,  4.0f, 0.0f });
			pathing->Speed = 2.0f;
		}

		GameObject obj6 = scene->CreateEntity("following_monkey");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(reflectiveMat);
			obj6.get<Transform>().SetLocalPosition(0.0f, 0.0f, 3.0f);
			obj6.get<Transform>().SetParent(obj4);
			
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj6);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 0.0f, 1.0f });
			pathing->Points.push_back({ 0.0f, 0.0f, 3.0f });
			pathing->Speed = 2.0f;
		}*/
		
		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 5, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			//controllables.push_back(obj2);
			controllables.push_back(obj3);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;
			

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;
						
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});



			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}