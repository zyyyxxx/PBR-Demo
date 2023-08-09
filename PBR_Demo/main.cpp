#include "utilities.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main()
{
#pragma region ��ʼ�� ���ûص����� IMGUI



	// glfw: initialize and configure
	// ------------------------------
	glfwInit();

	// ������Ҫ�ʹ�Ҫ�汾
	const char* glsl_version = "#version 330";


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PBR-Demo", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);



	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}



	// -----------------------
	// ����imgui������
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	// ������ʽ
	ImGui::StyleColorsDark();
	// ����ƽ̨����Ⱦ��
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// -----------------------
#pragma endregion

#pragma region OpenGL���ã���ȼ��ȣ� 

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	// set depth function to less than AND equal for skybox depth trick.
	glDepthFunc(GL_LEQUAL);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map. ��ʾ��������ͼ�ı߽��������������Բ�ֵ
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	//glEnable(GL_CULL_FACE);

#pragma endregion

#pragma region ����Shader ������ͼ���

	
	// PBR
	Shader equirectangularToCubemapShader("pbr/cubemap.vs", "pbr/equirectangular_to_cubemap.fs");
	Shader irradianceShader("pbr/cubemap.vs", "pbr/irradiance_convolution.fs");
	Shader prefilterShader("pbr/cubemap.vs", "pbr/prefilter.fs");
	Shader brdfShader("pbr/brdf.vs", "pbr/brdf.fs");
	Shader backgroundShader("pbr/background.vs", "pbr/background.fs");

	// �ӳ���Ⱦ
	Shader pbrShader("deffered_shading/deffered_shading.vs", "deffered_shading/deffered_shading.fs"); // ��ɫ
	Shader GeometryPass("deffered_shading/g_buffer.vs", "deffered_shading/g_buffer.fs");
	Shader shaderLightBox("deffered_shading/deferred_light_box.vs", "deffered_shading/deferred_light_box.fs");

	// ��Ӱӳ��
	Shader simpleDepthShader("shadow_mapping/point_shadows_depth.vs" , "shadow_mapping/point_shadows_depth.fs" , "shadow_mapping/point_shadows_depth.gs");

	backgroundShader.use();
	backgroundShader.setInt("environmentMap", 0);
	backgroundShader.setInt("DebugShadowCubemap", 1);
#pragma endregion

#pragma region ����PBR�ⲿ��ͼ ���ù�Դλ������ɫ


	// ����PBR��ͼ
	// --------------------------
	// rusted iron
	unsigned int ironAlbedoMap = loadTexture("image/pbr/rusted_iron/rusted_iron_albedo.png");
	unsigned int ironNormalMap = loadTexture("image/pbr/rusted_iron/rusted_iron_normal.png");
	unsigned int ironMetallicMap = loadTexture("image/pbr/rusted_iron/rusted_iron_Metallic.png");
	unsigned int ironRoughnessMap = loadTexture("image/pbr/rusted_iron/rusted_iron_roughness.png");
	unsigned int ironAOMap = loadTexture("image/pbr/rusted_iron/rusted_iron_ao.png");

	//gold
	unsigned int goldAlbedoMap = loadTexture("image/pbr/plastic/plastic_albedo.png");
	unsigned int goldNormalMap = loadTexture("image/pbr/plastic/plastic_normal.png");
	unsigned int goldMetallicMap = loadTexture("image/pbr/plastic/plastic_Metallic.png");
	unsigned int goldRoughnessMap = loadTexture("image/pbr/plastic/plasitc_roughness.png");
	unsigned int goldAOMap = loadTexture("image/pbr/plastic/plastic_ao.png");

	//grass
	unsigned int grassAlbedoMap = loadTexture("image/pbr/grass/grass_albedo.png");
	unsigned int grassNormalMap = loadTexture("image/pbr/grass/grass_normal.png");
	unsigned int grassMetallicMap = loadTexture("image/pbr/grass/grass_Metallic.png");
	unsigned int grassRoughnessMap = loadTexture("image/pbr/grass/grass_roughness.png");
	unsigned int grassAOMap = loadTexture("image/pbr/grass/grass_ao.png");


	// lighting info
	// -------------
	#define NR_LIGHTS  4
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(13);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5)*300.f; // between 0.5 and 1.)
		float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5)*300.f; // between 0.5 and 1.)
		float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5)*300.f; // between 0.5 and 1.)
		//float rColor = 300.0f; // between 0.5 and 1.)
		//float gColor = 300.0f; // between 0.5 and 1.)
		//float bColor = 300.0f; // between 0.5 and 1.)
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

#pragma endregion

#pragma region ����PBR:���ݼ���HDR��ͼ��֡������Ⱦ��envCubeMap �������������˲������淴��Ԥ�˲��Լ���պУ�

	// pbr: ����֡����
	// ----------------------
	unsigned int captureFBO;
	unsigned int captureRBO;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	// pbr: ����HDR������ͼ
	// ---------------------------------
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf("image/hdr/lake_pier_4k.hdr", &width, &height, &nrComponents, 0);
	unsigned int hdrTexture;
	if (data)
	{
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}

	// pbr: ����������Ⱦ��HDR CubeMap
	// ---------------------------------------------------------
	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // ����mipmap�Կ�Ԥ�˲����������
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// pbr: ����ͶӰ��view�������Բ���cubemap��������
	// ----------------------------------------------------------------------------------------------
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// pbr: ת��HDR��cubemap
	// ----------------------------------------------------------------------
	equirectangularToCubemapShader.use();
	equirectangularToCubemapShader.setInt("equirectangularMap", 0);
	equirectangularToCubemapShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 512, 512); // ���Ĳ������ڴ�С ֻ��Ҫ512
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO); // ��Ⱦ������֡������
	for (unsigned int i = 0; i < 6; ++i)
	{
		equirectangularToCubemapShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0); //����ͼ����ǰ����֡����
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ��OpenGL����mipmap�Կ�Ԥ�˲�������������㣬ԭʼ�Ĳ���ֻ��һ��envCubemap�ϲ��������۴ֲ�ֵ������ʹ��mipmap���зּ������ݴֲ�ֵѡ��ͬ��mipmap
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

#pragma endregion

#pragma region ����PBR:��������������ķ����irradianCecubemap


	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------
	unsigned int irradianceMap;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	irradianceShader.use();
	irradianceShader.setInt("environmentMap", 0);
	irradianceShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		irradianceShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

#pragma region ����PBR:�������淴��Ԥ�˲���ͼ


	// pbr: ����Ԥ�˲���ͼ������FBO��size����ΪԤ�˲���scale
	// --------------------------------------------------------------------------------
	unsigned int prefilterMap;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // ����mipamp�����Թ���
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// ����mipmap
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: �������ؿ���ģ�����Ԥ�˲���ͼ�����mipmapӦ�Բ�ͬroughness��
	// ----------------------------------------------------------------------------------------------------
	prefilterShader.use();
	prefilterShader.setInt("environmentMap", 0);
	prefilterShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// ������Ӧmipmap��fbo size
		unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader.setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			prefilterShader.setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderCube();
		}
	}
	

#pragma endregion

#pragma region ����PBR:����BRDFLUT(��BRFD������n*wi(��Χ��0.0��1.0֮��)��Ϊ������,�Դֲڶ���Ϊ������,�洢�Ƿ�������Ӧ��ϵ��(Rͨ��)��ƫ��ֵ(Gͨ��))
	
	// pbr: generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------
	unsigned int brdfLUTTexture;
	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	brdfShader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

#pragma region Deffered Shading: ��ʼ��GemortyPass֡����

	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));

	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// ������Ԥ���� ���漸����Ϣ���ĸ���ͼ��
	unsigned int gPosition, gNormal, gAlbedo , gMetallic_Roughness_AO;
	// λ�� color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// ���� color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// albedoMap  color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	// metalli_roughness_ao   color buffer �ֱ𴢴���RGB
	glGenTextures(1, &gMetallic_Roughness_AO);
	glBindTexture(GL_TEXTURE_2D, gMetallic_Roughness_AO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMetallic_Roughness_AO, 0);



	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering  ����OpenGL���ǽ�Ҫʹ��(֡�����)������ɫ������������Ⱦ
	unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer) ��ȸ���
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// ���֡�����Ƿ񴴽��ɹ�
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Shadow Mapping �������Դ������Ӱcubemap��ͼ ����������ͶӰ����

	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// ������Ӱ��ͼ����
	unsigned int depthCubemaps[NR_LIGHTS];
	glGenTextures(NR_LIGHTS, depthCubemaps);

	// ������Դ����Ӱ��ͼ
	for (int i = 0; i < NR_LIGHTS; i++) {
		// ��ÿ����ͼ
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);

		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
				0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
			
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	}
	
	// ��Ӱ��ͼ����
	float near_plane = 1.0f;
	float far_plane = 25.0f;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
	std::vector<std::vector<glm::mat4>> shadowTransforms(NR_LIGHTS);
	int i = 0;
	for (auto lightPos : lightPositions) {
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		shadowTransforms[i].push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		i++;
	}
	

#pragma endregion


#pragma region ��ʼ�������ͶӰ���� ��Դλ��

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	pbrShader.use();
	pbrShader.setMat4("projection", projection);
	
	pbrShader.setInt("NR_LIGHTS", NR_LIGHTS);
	// ���ݹ�Դ��Ϣ��fs
	for (unsigned int i = 0; i < lightPositions.size(); i++)
	{
		pbrShader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
		pbrShader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
		// update attenuation parameters and calculate radius
		const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
		const float linear = 0.7f;
		const float quadratic = 1.8f;
		pbrShader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
		pbrShader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
		// then calculate radius of light volume/sphere
		const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
		// ������뾶
		float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
		pbrShader.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
	}
	backgroundShader.use();
	backgroundShader.setMat4("projection", projection);
#pragma endregion


	// ��ԭ���ڴ�С
	int scrWidth, scrHeight;
	glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
	glViewport(0, 0, scrWidth, scrHeight);

	// ��Ⱦѭ��
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// ֡����Ϣ
		// --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;


		// ֡����Ϣ
		// *************************************************************************
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin("FPS");

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImGui::End();
		}
		// *************************************************************************


		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma region ��Ⱦÿ����Դ����Ӱ��ͼ

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glDrawBuffer(GL_NONE);//������Ⱦ����ɫ����
		glReadBuffer(GL_NONE);
		simpleDepthShader.use();

		// ����ÿ����Դ��Ⱦ��ͬ����Ӱ��ͼ
		for (int i = 0; i < NR_LIGHTS; i++) {
			// ����Ҫ����Ķ�Ӧcubemap
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemaps[i], 0);

			for (unsigned int j = 0; j < 6; ++j) {
				simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[i][j]);
			}

			simpleDepthShader.setFloat("far_plane", far_plane);
			simpleDepthShader.setVec3("lightPos", lightPositions[i]);

			// ��Ⱦ������
			for (unsigned int i = 0; i < objectPositions.size(); i++)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, objectPositions[i]);
				model = glm::scale(model, glm::vec3(0.25f));
				simpleDepthShader.setMat4("model", model);
				renderSphere();
			}

			//����

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0, -0.5, 0));
			model = glm::scale(model, glm::vec3(1.0f));
			simpleDepthShader.setMat4("model", model);
			renderFloor();

			// ��

			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0, 1, 0));
			model = glm::scale(model, glm::vec3(0.25f));
			simpleDepthShader.setMat4("model", model);
			renderSphere();

		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#pragma endregion

#pragma region 1. geometry pass ��Ⱦ��Ϣ��gbuffer
		
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		GeometryPass.use();
		GeometryPass.setMat4("projection", projection);
		GeometryPass.setMat4("view", view);

		// ����������pbr����
		GeometryPass.setInt("albedoMap", 0);
		GeometryPass.setInt("normalMap", 1);
		GeometryPass.setInt("metallicMap", 2);
		GeometryPass.setInt("roughnessMap", 3);
		GeometryPass.setInt("aoMap", 4);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ironNormalMap);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ironMetallicMap);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ironAOMap);

		// ��Ⱦ������
		for (unsigned int i = 0; i < objectPositions.size(); i++) 
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, objectPositions[i]);
			model = glm::scale(model, glm::vec3(0.25f));
			GeometryPass.setMat4("model", model);
			renderSphere();
		}

		// �󶨵���pbr����
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassAlbedoMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, grassNormalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, grassMetallicMap);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, grassRoughnessMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, grassAOMap);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0,-0.5,0));
		model = glm::scale(model, glm::vec3(0.5f));
		GeometryPass.setMat4("model", model);
		renderFloor();

		// ��
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, goldAlbedoMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, goldNormalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, goldMetallicMap);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, goldRoughnessMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, goldAOMap);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(0.25f));
		GeometryPass.setMat4("model", model);
		renderSphere();



		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion 
	
#pragma region 2. lighting pass
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		pbrShader.use();
		pbrShader.setInt("gPosition", 0);
		pbrShader.setInt("gNormal", 1);
		pbrShader.setInt("gAlbedo", 2);
		pbrShader.setInt("gMetallic_Roughness_AO", 3);
		pbrShader.setInt("irradianceMap", 4);
		pbrShader.setInt("prefilterMap", 5);
		pbrShader.setInt("brdfLUT", 6);

		// ��gbuffer��ͼ
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);//��g-buffer���������
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);//��g-buffer���������
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);//��g-buffer���������
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gMetallic_Roughness_AO);//��g-buffer���������

		// ��IBL��ͼ
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

		// ����Ӱ��ͼ
		
		// ���� depthCubemaps �� Uniform ����
		GLint depthCubemapsLocation = glGetUniformLocation(pbrShader.ID, "depthCubemaps");
		
		GLint depthCubemapIndices[NR_LIGHTS];

		// �������Ԫ������ depthCubemapIndices ����
		for (int i = 0; i < NR_LIGHTS; ++i)
		{
			pbrShader.setInt("depthCubemaps[" + std::to_string(i) + "]", 7+i);
		}
		

		for (int i = 0; i < NR_LIGHTS; ++i)
		{
			glActiveTexture(GL_TEXTURE7 + i);
			glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
		}

		pbrShader.setBool("shadows" , shadows);
		pbrShader.setVec3("viewPos", camera.Position);
		// ��Ⱦ����Ļ�ı���
		renderQuad();

		// ���������Ϣ��Ĭ�ϻ�����
		// ----------------------------------------------------------------------------------
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
		glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


#pragma endregion 

#pragma region ��Ⱦ��Դ����
		shaderLightBox.use();
		shaderLightBox.setMat4("projection", projection);
		shaderLightBox.setMat4("view", view);
		for (unsigned int i = 0; i < lightPositions.size(); i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPositions[i]);
			model = glm::scale(model, glm::vec3(0.125f));
			shaderLightBox.setMat4("model", model);
			shaderLightBox.setVec3("lightColor", lightColors[i]);
			renderSphere();
		}
#pragma endregion

#pragma region ��Ⱦ��պ�

		// render skybox (render as last to prevent overdraw)
		backgroundShader.use();

		backgroundShader.setMat4("view", view);
		backgroundShader.setBool("Debug", shadows);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[0]);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
		//glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
		renderCube();

#pragma endregion

		// render BRDF map to screen ��Ⱦbrdf��ͼ
		//brdfShader.use();
		//renderQuad();

		 // ��Ⱦ gui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


