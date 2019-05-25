#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "KernelTypes.hpp"
#include "KernelFunction.hpp"
#include "Model.hpp"

#include "ManagedFile.hpp"
#include "ManagedUtil.hpp"

#include "Profiler.hpp"
#include "Random.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define strdup _strdup
#endif

void ParseInt(std::string arg, const char* str, const char* var, int& dst)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto val = stoi(arg.substr(len));

				fprintf(stderr, "... %s = %d\n", var, val);

				dst = val;
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = NaN %s\n", var, ia.what());
				exit(1);
			}
		}
	}
}

void ParseInts(std::string arg, const char* str, const char* var, std::vector<int>& ints)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto s = arg.substr(len);

				std::string delimiter = ",";

				size_t pos = 0;

				while ((pos = s.find(delimiter)) != std::string::npos) {

					auto val = stoi(s.substr(0, pos));

					ints.push_back(val);

					s.erase(0, pos + delimiter.length());
				}

				if (s.length() > 0)
				{
					auto val = stoi(s.substr(0, pos));

					ints.push_back(val);
				}
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = NaN %s\n", var, ia.what());
				exit(1);
			}
		}
	}
}

void ParseDouble(std::string arg, const char* str, const char* var, double& dst)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto val = stod(arg.substr(len));

				fprintf(stderr, "... %s = %e\n", var, val);

				dst = val;
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = NaN %s\n", var, ia.what());

				exit(1);
			}
		}
	}
}

void ParseDoubles(std::string arg, const char* str, const char* var, std::vector<double>& doubles)
{
	auto len = strlen(str);

	if (len > 0)
	{
		if (!arg.compare(0, len, str) && arg.length() > len)
		{
			try
			{
				auto s = arg.substr(len);

				std::string delimiter = ",";

				size_t pos = 0;

				while ((pos = s.find(delimiter)) != std::string::npos) {

					auto val = stof(s.substr(0, pos));

					doubles.push_back(val);

					s.erase(0, pos + delimiter.length());
				}

				if (s.length() > 0)
				{
					auto val = stof(s.substr(0, pos));

					doubles.push_back(val);
				}
				
				if (!doubles.empty())
				{
					std::cout << "... " << var << " = ";
					
					// Convert all but the last element to avoid a trailing ","
					for (auto i = 0; i < doubles.size(); i++)
					{
						if (i > 0)
							std::cout << ", ";
						
						std::cout << doubles[i];
					}
					
					// Now add the last element with no delimiter
					std::cout << std::endl;
				}
			}
			catch (const std::invalid_argument& ia)
			{
				fprintf(stderr, "... %s = NaN %s\n", var, ia.what());
				exit(1);
			}
		}
	}
}

void Load2D(std::string filename, ManagedArray& input, ManagedArray& output, const char* delimiter, int& inputs, int& categories, int& examples)
{
	auto temp = std::vector<std::vector<double>>();

	std::ifstream file(filename);
	std::string line;
	
	categories = 0;
	inputs = 0;
	examples = 0;
	
	while (std::getline(file, line))
	{
		if (std::strlen(line.c_str()) > 0)
		{
			temp.push_back(std::vector<double>());
			
			auto current_line = strdup(line.c_str());
			
			char* next_token = NULL;

			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
				auto token = std::strtok_s(current_line, delimiter, &next_token);
			
			#else
			
				auto token = std::strtok(current_line, delimiter);
			
			#endif
			
			int tokens = 0;
			
			while (token != NULL)
			{
				tokens++;
				
				auto value = atof(token);
				
				temp[examples].push_back(value);
				
				#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
				
					token = std::strtok_s(NULL, delimiter, &next_token);
				
				#else
			
					token = strtok(NULL, delimiter);
			
				#endif
			}
		
			free(current_line);
		
			if (tokens > 0)
				examples++;
		}
	}

	auto sizey = (int)temp.size();
	auto sizex = (int)temp[0].size();
	inputs = sizex - 1;

	input.Resize(inputs, sizey, false);
	output.Resize(1, sizey, false);
	
	for (auto y = 0; y < sizey; y++)
	{
		for (auto x = 0; x < sizex; x++)
		{
			auto category = inputs > 0 ? (int)temp[y][inputs] : 0;
			
			categories = std::max(categories, category);
			
			if (x < inputs)
				input(x, y) = temp[y][x];
				
			output(y) = category;
		}
	}
	
	file.close();
}

void Load2D(std::string filename, ManagedArray& input, const char* delimiter, int features, int& samples)
{
	auto temp = std::vector<std::vector<double>>();

	std::ifstream file(filename);
	std::string line;
	
	samples = 0;
	
	while (std::getline(file, line))
	{
		if (std::strlen(line.c_str()) > 0)
		{
			temp.push_back(std::vector<double>());
			
			auto current_line = strdup(line.c_str());
			
			char* next_token = NULL;

			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
				auto token = std::strtok_s(current_line, delimiter, &next_token);
			
			#else
			
				auto token = std::strtok(current_line, delimiter);
			
			#endif
			
			int tokens = 0;
			
			while (token != NULL)
			{
				tokens++;
				
				auto value = atof(token);
				
				temp[samples].push_back(value);
				
				#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
				
					token = std::strtok_s(NULL, delimiter, &next_token);
				
				#else
			
					token = strtok(NULL, delimiter);
			
				#endif
			}
		
			free(current_line);
		
			if (tokens > 0 && tokens >= features)
				samples++;
		}
	}

	auto sizey = (int)temp.size();
	auto sizex = (int)temp[0].size();

	input.Resize(features, sizey, false);
	
	for (auto y = 0; y < sizey; y++)
	{
		for (auto x = 0; x < sizex; x++)
		{
			if (x >= 0 && x < features && (int)temp[y].size() >= features)
				input(x, y) = temp[y][x];
		}
	}
	
	file.close();
}

void SVMTrainer(std::string InputData, int delimiter, KernelType kernel, std::vector<double> kernelParams, int category, double c, int passes, double tolerance, bool save, std::string SaveDirectory, std::string SaveJSON)
{
	std::string BaseDirectory = "./";
	
	if (std::strlen(InputData.c_str()) > 0)
	{
		auto Inputs = 0;
		auto Categories = 0;
		auto Examples = 0;
		
		auto input = ManagedArray();
		auto output = ManagedArray();
		
		Load2D(InputData, input, output, delimiter == 0 ? "\t" : ",", Inputs, Categories, Examples);
		
		fprintf(stderr, "\n%d lines read with %d inputs and %d categories\n", Examples, Inputs, Categories);
		
		if (Inputs > 0 && Categories > 0 && Examples > 0 && kernel != KernelType::UNKNOWN)
		{
			if (category > 0 && category <= Categories)
			{
				auto params = ManagedArray((int)kernelParams.size());
				
				for (auto i = 0; i < params.Length(); i++)
				{
					params(i) = kernelParams[i];
				}
				
				auto start = Profiler::now();
				
				auto model = Model();
				
				model.GetNormalization(input);
				
				fprintf(stderr, "\nTraining Model...\n");
				
				model.Train(input, output, c, kernel, params, tolerance, passes, category);
				
				fprintf(stderr, "Training Done\n");

				fprintf(stderr, "elapsed time is %ld ms\n", Profiler::Elapsed(start));

				if (save && std::strlen(SaveJSON.c_str()) > 0)
				{
					fprintf(stderr, "\nSaving Model Parameters\n");

					ManagedFile::SaveJSON(SaveDirectory.empty() ? BaseDirectory : SaveDirectory, SaveJSON, ManagedUtil::Serialize(model));
				}
				
				ManagedOps::Free(params);
				
				model.Free();
			}
			else
			{
				auto params = ManagedArray((int)kernelParams.size());
				
				for (auto i = 0; i < params.Length(); i++)
				{
					params(i) = kernelParams[i];
				}
				
				std::vector<Model> models;
				
				auto start = Profiler::now();
				
				for (auto i = 0; i < Categories; i++)
				{
					auto model = Model();
					
					model.GetNormalization(input);
					model.Setup(input, output, c, kernel, params, tolerance, passes, i + 1);
					models.push_back(model);
				}
				
				auto done = false;
				
				fprintf(stderr, "\nTraining Models...\n");
				
				while (!done)
				{
					done = true;
					
					for (auto i = 0; i < models.size(); i++)
					{
						auto result = models[i].Step();
					
						if (result && !models[i].Trained)
						{
							models[i].Generate();
						}
						
						done &= result;
					}
				}
				
				fprintf(stderr, "Training Done\n");
				
				fprintf(stderr, "elapsed time is %ld ms\n", Profiler::Elapsed(start));
				
				if (save && std::strlen(SaveJSON.c_str()) > 0)
				{
					fprintf(stderr, "\nSaving Model Parameters\n");

					ManagedFile::SaveJSON(SaveDirectory.empty() ? BaseDirectory : SaveDirectory, SaveJSON, ManagedUtil::Serialize(models));
				}
				
				ManagedOps::Free(params);
				
				for (auto i = 0; i < models.size(); i++)
				{
					models[i].Free();
				}	
			}
		}
		
		ManagedOps::Free(input);
		ManagedOps::Free(output);
	}
}

void SVMPredict(std::string InputData, std::string ModelFile, int delimiter, int Features, bool save, std::string SaveDirectory, std::string ClassificationFile)
{
	std::string BaseDirectory = "./";
	
	if (std::strlen(InputData.c_str()) > 0)
	{
		auto Samples = 0;
		
		auto input = ManagedArray();
		
		Load2D(InputData, input, delimiter == 0 ? "\t" : ",", Features, Samples);
		
		fprintf(stderr, "\n%d lines read with %d features\n", Samples, Features);
		
		if (Features > 0 && Samples > 0)
		{
			auto models = ManagedUtil::Deserialize(ModelFile);
			auto prediction = ManagedArray(1, Samples);
			auto classification = ManagedIntList(Samples);
			ManagedOps::Set(classification, 0);

			fprintf(stderr, "\nClassifying input data...\n");
			
			auto start = Profiler::now();

			for (auto i = 0; i < (int)models.size(); i++)
			{
				fprintf(stderr, "\nUsing model %d...\n", i + 1);

				auto p = models[i].Predict(input);

				for (auto y = 0; y < p.Length(); y++)
				{
					if (p(y) > prediction(y))
					{
						prediction(y) = p(y);
						classification(y) = models[i].Category;
					}
				}

				ManagedOps::Free(p);

				models[i].Free();
			}

			fprintf(stderr, "\nClassification:\n");
			ManagedMatrix::PrintList(classification, true);
			
			fprintf(stderr, "\nClassification Done\n");
			fprintf(stderr, "elapsed time is %ld ms\n", Profiler::Elapsed(start));

			if (save && std::strlen(ClassificationFile.c_str()) > 0)
			{
				ManagedFile::SaveClassification(SaveDirectory.empty() ? BaseDirectory : SaveDirectory, ClassificationFile, classification);
			}

			ManagedOps::Free(prediction);
			ManagedOps::Free(classification);
		}
		
		ManagedOps::Free(input);
	}
}

int main(int argc, char** argv)
{
	// Training
	auto passes = 5;
	auto c = 1.0;
	auto tolerance = 0.0001;
	auto type = KernelType::UNKNOWN;
	auto category = 0;
	std::vector<double> parameters;

	// Prediction
	auto predict = false;
	auto features = 0;

	// Files	
	auto save = false;
	
	char SaveDirectory[200];
	SaveDirectory[0] = '\0';

	std::string SaveDir;

	char SaveJSON[200];
	SaveJSON[0] = '\0';

	char InputData[200];
	InputData[0] = '\0';
	
	char ModelFile[200];
	ModelFile[0] = '\0';

	char ClassificationFile[200];
	ClassificationFile[0] = '\0';

	int delimiter = 0;

	for (auto i = 0; i < argc; i++)
	{
		std::string arg = argv[i];
		std::transform(arg.begin(), arg.end(), arg.begin(), ::toupper);

		if (!arg.compare("/SAVE"))
		{
			save = true;
		}
		else if (!arg.compare("/POLYNOMIAL"))
		{
			type = KernelType::POLYNOMIAL;
			
			fprintf(stderr, "... Kernel type = Polynomial\n");
		}
		else if (!arg.compare("/GAUSSIAN"))
		{
			type = KernelType::GAUSSIAN;
			
			fprintf(stderr, "... Kernel type = Gaussian\n");
		}
		else if (!arg.compare("/RADIAL"))
		{
			type = KernelType::RADIAL;
			
			fprintf(stderr, "... Kernel type = Radial basis functions\n");
		}
		else if (!arg.compare("/SIGMOID"))
		{
			type = KernelType::SIGMOID;
			
			fprintf(stderr, "... Kernel type = Sigmoid\n");
		}
		else if (!arg.compare("/LINEAR"))
		{
			type = KernelType::LINEAR;
			
			fprintf(stderr, "... Kernel type = Linear\n");
		}
		else if (!arg.compare("/FOURIER"))
		{
			type = KernelType::FOURIER;
			
			fprintf(stderr, "... Kernel type = Fourier basic functions\n");
		}
		else if (!arg.compare("/TAB"))
		{
			delimiter = 0;
		}
		else if (!arg.compare("/COMMA"))
		{
			delimiter = 1;
		}
		else if (!arg.compare("/PREDICT"))
		{
			predict = true;
		}

		if (!arg.compare(0, 9, "/SAVEDIR=") && arg.length() > 9)
		{
			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

				strncpy_s(SaveDirectory, &argv[i][9], sizeof(SaveDirectory));
			
			#else

				strncpy(SaveDirectory, &argv[i][9], sizeof(SaveDirectory));

			#endif
		}

		if (!arg.compare(0, 6, "/JSON=") && arg.length() > 6)
		{
			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
			strncpy_s(SaveJSON, &argv[i][6], sizeof(SaveJSON));

			#else
				
				strncpy(SaveJSON, &argv[i][6], sizeof(SaveJSON));

			#endif
		}
		
		if (!arg.compare(0, 7, "/INPUT=") && arg.length() > 7)
		{
			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
			strncpy_s(InputData, &argv[i][7], sizeof(InputData));

			#else
				
				strncpy(InputData, &argv[i][7], sizeof(InputData));

			#endif
		}

		if (!arg.compare(0, 7, "/MODEL=") && arg.length() > 7)
		{
			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
			strncpy_s(ModelFile, &argv[i][7], sizeof(ModelFile));

			#else
				
				strncpy(ModelFile, &argv[i][7], sizeof(ModelFile));

			#endif
		}

		if (!arg.compare(0, 5, "/TXT=") && arg.length() > 5)
		{
			#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			
			strncpy_s(ClassificationFile, &argv[i][5], sizeof(ClassificationFile));

			#else
				
				strncpy(ClassificationFile, &argv[i][5], sizeof(ClassificationFile));

			#endif
		}

		ParseInt(arg, "/PASSES=", "Max # of passes", passes);
		ParseInt(arg, "/CATEGORY=", "Category", category);
		ParseInt(arg, "/FEATURES=", "# features per data point", features);
		ParseDouble(arg, "/TOLERANCE=", "Error tolerance", tolerance);
		ParseDouble(arg, "/C=", "Regularization constant", c);
		ParseDoubles(arg, "/PARAMETERS=", "Kernel Parameters", parameters);
	}

	if (strlen(SaveDirectory) > 0 && save)
	{

		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

			SaveDir = "./" + std::string(SaveDirectory) + "/";

		#else

			SaveDir = "./" + std::string(SaveDirectory);

		#endif

		fprintf(stderr, "... Save Directory: %s\n", SaveDirectory);
	}
	
	if (strlen(InputData) > 0)
	{
		fprintf(stderr, "... Input training data: %s\n", InputData);
	}
	
	if (strlen(ModelFile) > 0)
	{
		fprintf(stderr, "... Model File: %s\n", ModelFile);
	}

	if (strlen(SaveJSON) > 0)
	{
		fprintf(stderr, "... JSON File: %s.json\n", SaveJSON);
	}

	if (strlen(ClassificationFile) > 0)
	{
		fprintf(stderr, "... Classification File: %s.txt\n", ClassificationFile);
	}

	if (predict)
	{
		SVMPredict(InputData, ModelFile, delimiter, features, save, SaveDirectory, ClassificationFile);
	}
	else
	{
		SVMTrainer(InputData, delimiter, type, parameters, category, c, passes, tolerance, save, SaveDirectory, SaveJSON);
	}
		
	return 0;
}
