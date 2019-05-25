#ifndef MODEL_HPP
#define MODEL_HPP

#include <algorithm>
#include <cmath>
#include <vector>

#include "KernelFunction.hpp"
#include "Random.hpp"

class Model
{
private:

	// Internal variables
	ManagedArray K = NULL;
	ManagedArray E = NULL;
	ManagedArray alpha = NULL;
	ManagedArray dx = NULL;
	ManagedArray dy = NULL;
	ManagedArray kparam = NULL;
	double b = 0.0;
	double eta = 0.0;
	double H = 0.0;
	double L = 0.0;
	KernelType ktype = KernelType::UNKNOWN;
        
public:

	ManagedArray ModelX = NULL;
	ManagedArray ModelY = NULL;
	KernelType Type = KernelType::UNKNOWN;
	ManagedArray KernelParam = NULL;
	ManagedArray Alpha = NULL;
	ManagedArray W = NULL;
	double B = 0.0;
	double C = 1.0;
	double Tolerance;
	int Category = 0;
	int Passes = 5;
	int Iterations = 0;
	int MaxIterations = 5;
	bool Trained = false;
	
	std::vector<double> Min;
	std::vector<double> Max;
	
	Random random = Random();
	
	Model()
	{

	}

	Model(ManagedArray& x, ManagedArray& y, KernelType type, ManagedArray& kernelParam, ManagedArray& alpha, ManagedArray& w, double b, double c, double tolerance, int category, int passes)
	{
		ModelX = x;
		ModelY = y;
		Type = type;
		KernelParam = kernelParam;
		Alpha = alpha;
		W = w;
		B = b;
		C = c;
		Tolerance = tolerance;
		Category = category;
		Passes = passes;
		MaxIterations = passes;
		Iterations = passes;
		Trained = true;
	}

	int Rows(ManagedArray& x)
	{
		return x.y;
	}

	int Cols(ManagedArray& x)
	{
		return x.x;
	}
	
	void Setup(ManagedArray& x, ManagedArray& y, double c, KernelType kernel, ManagedArray& param, double tolerance = 0.001, int maxpasses = 5, int category = 1)
	{
		ManagedOps::Free(dx);
		ManagedOps::Free(dy);
		
		dx = ManagedArray(x.x, x.y);
		dy = ManagedArray(y.x, y.y);

		ManagedOps::Copy2D(dx, x, 0, 0);
		ManagedOps::Copy2D(dy, y, 0, 0);

		ktype = kernel;

		// Data parameters
		auto m = Rows(dx);

		Category = category;
		MaxIterations = maxpasses;
		Tolerance = tolerance;
		C = c;

		// Reset internal variables
		ManagedOps::Free(K);
		ManagedOps::Free(kparam);
		ManagedOps::Free(E);
		ManagedOps::Free(alpha);

		kparam = ManagedArray(param.Length());
		ManagedOps::Copy2D(kparam, param, 0, 0);

		// Variables
		alpha = ManagedArray(1, m);
		E = ManagedArray(1, m);
		b = 0.0;
		Iterations = 0;

		// Pre-compute the Kernel Matrix since our dataset is small
		// (In practice, optimized SVM packages that handle large datasets
		// gracefully will *not* do this)
		if (kernel == KernelType::LINEAR)
		{
			// Computation for the Linear Kernel
			// This is equivalent to computing the kernel on every pair of examples
			auto tinput = ManagedMatrix::Transpose(dx);

			K = ManagedMatrix::Multiply(dx, tinput);

			double slope = kparam.Length() > 0 ? kparam(0) : 1;
			double inter = kparam.Length() > 1 ? kparam(1) : 0;

			ManagedMatrix::Multiply(K, slope);
			ManagedMatrix::Add(K, inter);

			ManagedOps::Free(tinput);
		}
		else if (kernel == KernelType::GAUSSIAN || kernel == KernelType::RADIAL)
		{
			// RBF Kernel
			// This is equivalent to computing the kernel on every pair of examples
			auto pX2 = ManagedMatrix::Pow(dx, 2);
			auto rX2 = ManagedMatrix::RowSums(pX2);
			auto tX2 = ManagedMatrix::Transpose(rX2);
			auto trX = ManagedMatrix::Transpose(dx);

			auto tempK = ManagedArray(m, m);
			auto temp1 = ManagedArray(m, m);
			auto temp2 = ManagedMatrix::Multiply(dx, trX);

			ManagedMatrix::Expand(rX2, m, 1, tempK);
			ManagedMatrix::Expand(tX2, 1, m, temp1);
			ManagedMatrix::Multiply(temp2, -2);

			ManagedMatrix::Add(tempK, temp1);
			ManagedMatrix::Add(tempK, temp2);

			double sigma = kparam.Length() > 0 ? kparam(0) : 1;

			auto g = std::abs(sigma) > 0 ? std::exp(-1 / (2 * sigma * sigma)) : 0;

			if (Type == KernelType::RADIAL)
				ManagedMatrix::Sqrt(tempK);

			K = ManagedMatrix::Pow(g, tempK);

			ManagedOps::Free(pX2);
			ManagedOps::Free(rX2);
			ManagedOps::Free(tX2);
			ManagedOps::Free(trX);
			ManagedOps::Free(tempK);
			ManagedOps::Free(temp1);
			ManagedOps::Free(temp2);
		}
		else
		{
			// Pre-compute the Kernel Matrix
			// The following can be slow due to the lack of vectorization
			K = ManagedArray(m, m);

			auto Xi = ManagedArray(Cols(dx), 1);
			auto Xj = ManagedArray(Cols(dx), 1);

			for (auto i = 0; i < m; i++)
			{
				ManagedOps::Copy2D(Xi, dx, 0, i);

				for (auto j = 0; j < m; j++)
				{
					ManagedOps::Copy2D(Xj, dx, 0, j);

					K(j, i) = KernelFunction::Run(kernel, Xi, Xj, kparam);

					// the matrix is symmetric
					K(i, j) = K(j, i);
				}
			}

			ManagedOps::Free(Xi);
			ManagedOps::Free(Xj);
		}

		eta = 0.0;
		L = 0.0;
		H = 0.0;

		// Map 0 (or other categories) to -1
		for (auto i = 0; i < Rows(dy); i++)
		{
			dy(i) = (int)dy(i) != Category ? -1 : 1;
		}
		
		random.UniformDistribution();
	}
	
	void GetNormalization(ManagedArray& input)
	{
		Min.clear();
		Max.clear();

		for (int i = 0; i < input.x; i++)
		{
			Max.push_back(std::numeric_limits<double>::min());
			Min.push_back(std::numeric_limits<double>::max());
		}

		for (int y = 0; y < input.y; y++)
		{
			for (int x = 0; x < input.x; x++)
			{
				auto val = input(x, y);

				Max[x] = std::max(Max[x], val);
				Min[x] = std::min(Min[x], val);
			}
		}
	}
	
	ManagedArray Normalize(ManagedArray& input)
	{
		Min.clear();
		Max.clear();

		auto result = ManagedArray(input.x, input.y, false);

		for (int i = 0; i < input.x; i++)
		{
			Max.push_back(std::numeric_limits<double>::min());
			Min.push_back(std::numeric_limits<double>::max());
		}

		for (int y = 0; y < input.y; y++)
		{
			for (int x = 0; x < input.x; x++)
			{
				auto val = input(x, y);

				Max[x] = std::max(Max[x], val);
				Min[x] = std::min(Min[x], val);
			}
		}

		for (int y = 0; y < input.y; y++)
		{
			for (int x = 0; x < input.x; x++)
			{
				auto val = input(x, y);

				auto denum = Max[x] - Min[x];

				result(x, y) = (val - Min[x]) / denum;
			}
		}

		return result;
	}

	ManagedArray ApplyNormalization(ManagedArray& input)
	{
		auto result = ManagedArray(input.x, input.y, false);

		if (Min.size() > 0 && Max.size() > 0)
		{
			for (int y = 0; y < input.y; y++)
			{
				for (int x = 0; x < input.x; x++)
				{
					auto val = input(x, y);

					auto denum = Max[x] - Min[x];

					result(x, y) = (val - Min[x]) / denum;
				}
			}
		}
		else
		{
			for (int y = 0; y < input.y; y++)
			{
				for (int x = 0; x < input.x; x++)
				{
					result(x, y) = input(x, y);
				}
			}
		}

		return result;
	}
	
	bool Step()
	{
		if (Iterations >= MaxIterations)
			return true;

		// Data parameters
		auto m = Rows(dy);

		auto num_changed_alphas = 0;

		for (auto i = 0; i < m; i++)
		{
			// Calculate Ei = f(x(i)) - y(i) using (2).
			E(i) = b;

			for (auto yy = 0; yy < m; yy++)
			{
				E(i) += alpha(yy) * dy(yy) * K(yy, i);
			}

			E(i) -= dy(i);

			if ((dy(i) * E(i) < -Tolerance && alpha(i) < C) || (dy(i) * E(i) > Tolerance && alpha(i) > 0))
			{
				// In practice, there are many heuristics one can use to select
				// the i and j. In this simplified code, we select them randomly.
				auto j = i;

				while (j == i)
				{
					// Make sure i != j
					j = (int)std::floor(m * random.NextDouble());
				}

				// Calculate Ej = f(x(j)) - y(j) using (2).
				E(j) = b;

				for (auto yy = 0; yy < m; yy++)
				{
					E(j) += alpha(yy) * dy(yy) * K(yy, j);
				}

				E(j) -= dy(j);

				// Save old alphas
				auto alpha_i_old = alpha(i);
				auto alpha_j_old = alpha(j);

				// Compute L and H by (10) or (11). 
				if ((int)dy(i) == (int)dy(j))
				{
					L = std::max(0.0, alpha(j) + alpha(i) - C);
					H = std::min(C, alpha(j) + alpha(i));
				}
				else
				{
					L = std::max(0.0, alpha(j) - alpha(i));
					H = std::min(C, C + alpha(j) - alpha(i));
				}

				if (std::abs(L - H) <= std::numeric_limits<double>::epsilon())
				{
					// continue to next i 
					continue;
				}

				// Compute eta by (14).
				eta = 2 * K(j, i) - K(i, i) - K(j, j);

				if (eta >= 0)
				{
					// continue to next i. 
					continue;
				}

				// Compute and clip value for alpha j using (12) and (15).
				alpha(j) = alpha(j) - (dy(j) * (E(i) - E(j))) / eta;

				// Clip
				alpha(j) = std::min(H, alpha(j));
				alpha(j) = std::max(L, alpha(j));

				// Check if change in alpha is significant
				if (std::abs(alpha(j) - alpha_j_old) < Tolerance)
				{
					// continue to next i. 
					// replace anyway
					alpha(j) = alpha_j_old;

					continue;
				}

				// Determine value for alpha i using (16). 
				alpha(i) = alpha(i) + dy(i) * dy(j) * (alpha_j_old - alpha(j));

				// Compute b1 and b2 using (17) and (18) respectively. 
				auto b1 = b - E(i) - dy(i) * (alpha(i) - alpha_i_old) * K(j, i) - dy(j) * (alpha(j) - alpha_j_old) * K(j, i);
				auto b2 = b - E(j) - dy(i) * (alpha(i) - alpha_i_old) * K(j, i) - dy(j) * (alpha(j) - alpha_j_old) * K(j, j);

				// Compute b by (19). 
				if (0 < alpha(i) && alpha(i) < C)
				{
					b = b1;
				}
				else if (0 < alpha(j) && alpha(j) < C)
				{
					b = b2;
				}
				else
				{
					b = (b1 + b2) / 2;
				}

				num_changed_alphas++;
			}
		}

		if (num_changed_alphas == 0)
		{
			Iterations++;
		}
		else
		{
			Iterations = 0;
		}

		return Iterations >= MaxIterations;
	}
	
	void Generate()
	{
		auto m = Rows(dx);
		auto n = Cols(dx);

		auto idx = 0;

		for (auto i = 0; i < m; i++)
		{
			if (std::abs(alpha(i)) > 0)
			{
				idx++;
			}
		}

		ManagedOps::Free(ModelX);
		ManagedOps::Free(ModelY);
		ManagedOps::Free(Alpha);
		ManagedOps::Free(W);
		ManagedOps::Free(KernelParam);

		ModelX = ManagedArray(Cols(dx), idx);
		ModelY = ManagedArray(1, idx);
		Alpha = ManagedArray(1, idx);
		KernelParam = ManagedArray(kparam.Length());

		auto ii = 0;

		for (auto i = 0; i < m; i++)
		{
			if (std::abs(alpha(i)) > 0)
			{
				for (int j = 0; j < n; j++)
				{
					ModelX(j, ii) = dx(j, i);
				}

				ModelY(ii) = dy(i);

				Alpha(ii) = alpha(i);

				ii++;
			}
		}

		B = b;
		Passes = Iterations;
		ManagedOps::Copy2D(KernelParam, kparam, 0, 0);
		Type = ktype;

		auto axy = ManagedMatrix::BSXMUL(alpha, dy);
		auto tay = ManagedMatrix::Transpose(axy);
		auto txx = ManagedMatrix::Multiply(tay, dx);

		W = ManagedMatrix::Transpose(txx);

		Trained = true;

		ManagedOps::Free(dx);
		ManagedOps::Free(dy);
		ManagedOps::Free(K);
		ManagedOps::Free(kparam);
		ManagedOps::Free(E);
		ManagedOps::Free(alpha);
		ManagedOps::Free(axy);
		ManagedOps::Free(tay);
		ManagedOps::Free(txx);
	}

	// SVMTRAIN Trains an SVM classifier using a simplified version of the SMO 
	// algorithm.
	//
	// (model) = svm_train(X, Y, C, kernelFunction, kernelParam, tol, max_passes) trains an
	// SVM classifier and returns trained model. X is the matrix of training 
	// examples.  Each row is a training example, and the jth column holds the 
	// jth feature.  Y is a column matrix containing 1 for positive examples 
	// and 0 for negative examples.  C is the standard SVM regularization 
	// parameter.  tol is a tolerance value used for determining equality of 
	// floating point numbers. max_passes controls the number of iterations
	// over the dataset (without changes to alpha) before the algorithm quits.
	//
	// Note: This is a simplified version of the SMO algorithm for training
	// SVMs. In practice, if you want to train an SVM classifier, we
	// recommend using an optimized package such as:  
	//
	// LIBSVM   (http://www.csie.ntu.edu.tw/~cjlin/libsvm/)
	// SVMLight (http://svmlight.joachims.org/)
	//
	// Converted to R by: SD Separa (2016/03/18)
	// Converted to C# by: SD Separa (2018/09/29)
	//
	void Train(ManagedArray& x, ManagedArray& y, double c, KernelType kernel, ManagedArray& param, double tolerance = 0.001, int maxpasses = 5, int category = 1)
	{
		Setup(x, y, c, kernel, param, tolerance, maxpasses, category);

		// Train
		while (!Step()) { }

		Generate();
	}

	// SVMPREDICT returns a vector of predictions using a trained SVM model
	//(svm_train). 
	//
	// pred = SVMPREDICT(model, X) returns a vector of predictions using a 
	// trained SVM model (svm_train). X is a mxn matrix where there each 
	// example is a row. model is a svm model returned from svm_train.
	// predictions pred is a m x 1 column of predictions of {0, 1} values.
	//
	// Converted to R by: SD Separa (2016/03/18)
	// Converted to C# by: SD Separa (2018/09/29)
	ManagedArray Predict(ManagedArray& input)
	{
		auto predictions = ManagedArray(1, Rows(input));

		if (Trained)
		{
			auto x = ManagedArray(input.x, input.y, input.z, input.i, input.j, false);

			if (Cols(x) == 1)
			{
				ManagedMatrix::Transpose(x, input);
			}
			else
			{
				ManagedOps::Copy2D(x, input, 0, 0);
			}

			auto m = Rows(x);

			predictions.Resize(1, m);

			if (Type == KernelType::LINEAR)
			{
				ManagedMatrix::Multiply(predictions, x, W);
				ManagedMatrix::Add(predictions, B);
			}
			else if (Type == KernelType::GAUSSIAN || Type == KernelType::RADIAL)
			{
				// RBF Kernel
				// This is equivalent to computing the kernel on every pair of examples
				auto pX1 = ManagedMatrix::Pow(x, 2);
				auto pX2 = ManagedMatrix::Pow(ModelX, 2);
				auto rX2 = ManagedMatrix::RowSums(pX2);

				auto X1 = ManagedMatrix::RowSums(pX1);
				auto X2 = ManagedMatrix::Transpose(rX2);
				auto tX = ManagedMatrix::Transpose(ModelX);
				auto tY = ManagedMatrix::Transpose(ModelY);
				auto tA = ManagedMatrix::Transpose(Alpha);

				auto rows = Rows(X1);
				auto cols = Cols(X2);

				auto tempK = ManagedArray(rows, cols);
				auto temp1 = ManagedArray(cols, rows);
				auto temp2 = ManagedMatrix::Multiply(x, tX);

				ManagedMatrix::Multiply(temp2, -2);

				ManagedMatrix::Expand(X1, cols, 1, tempK);
				ManagedMatrix::Expand(X2, 1, rows, temp1);

				ManagedMatrix::Add(tempK, temp1);
				ManagedMatrix::Add(tempK, temp2);

				auto sigma = KernelParam.Length() > 0 ? KernelParam(0) : 1;

				if (Type == KernelType::RADIAL)
					ManagedMatrix::Sqrt(tempK);

				auto g = std::abs(sigma) > 0 ? std::exp(-1 / (2 * sigma * sigma)) : 0;

				auto Kernel = ManagedMatrix::Pow(g, tempK);

				auto tempY = ManagedArray(Cols(tY), rows);
				auto tempA = ManagedArray(Cols(tA), rows);

				ManagedMatrix::Expand(tY, 1, rows, tempY);
				ManagedMatrix::Expand(tA, 1, rows, tempA);

				ManagedMatrix::Product(Kernel, tempY);
				ManagedMatrix::Product(Kernel, tempA);

				auto p = ManagedMatrix::RowSums(Kernel);

				ManagedOps::Copy2D(predictions, p, 0, 0);
				ManagedMatrix::Add(predictions, B);

				ManagedOps::Free(pX1);
				ManagedOps::Free(pX2);
				ManagedOps::Free(rX2);
				ManagedOps::Free(X1);
				ManagedOps::Free(X2);
				ManagedOps::Free(tempK);
				ManagedOps::Free(temp1);
				ManagedOps::Free(temp2);
				ManagedOps::Free(tX);
				ManagedOps::Free(tY);
				ManagedOps::Free(tA);
				ManagedOps::Free(tempY);
				ManagedOps::Free(tempA);
				ManagedOps::Free(Kernel);
				ManagedOps::Free(p);
			}
			else
			{
				auto Xi = ManagedArray(Cols(x), 1);
				auto Xj = ManagedArray(Cols(ModelX), 1);

				for (auto i = 0; i < m; i++)
				{
					double prediction = 0.0;

					ManagedOps::Copy2D(Xi, x, 0, i);

					for (auto j = 0; j < Rows(ModelX); j++)
					{
						ManagedOps::Copy2D(Xj, ModelX, 0, j);

						prediction += Alpha(j) * ModelY(j) * KernelFunction::Run(Type, Xi, Xj, KernelParam);
					}

					predictions(i) = prediction + B;
				}

				ManagedOps::Free(Xi);
				ManagedOps::Free(Xj);
			}

			ManagedOps::Free(x);
		}

		return predictions;
	}

	ManagedIntList Classify(ManagedArray& input, double threshold = 0.0)
	{
		auto classification = ManagedIntList(Rows(input));

		auto predictions = Predict(input);

		for (auto i = 0; i < predictions.Length(); i++)
		{
			classification(i) = predictions(i) > threshold ? Category : 0;
		}

		ManagedOps::Free(predictions);

		return classification;
	}

	int Test(ManagedArray& output, ManagedIntList& classification, int category = 1)
	{
		auto errors = 0;

		for (auto i = 0; i < classification.Length(); i++)
		{
			auto correct = (int)output(i) != category ? 0 : category;

			errors += correct != classification(i) ? 1 : 0;
		}

		return errors;
	}

	void Free()
	{
		// variables
		ManagedOps::Free(ModelX);
		ManagedOps::Free(ModelY);
		ManagedOps::Free(Alpha);
		ManagedOps::Free(W);
		ManagedOps::Free(KernelParam);

		// internal variables
		ManagedOps::Free(K);
		ManagedOps::Free(E);
		ManagedOps::Free(alpha);
		ManagedOps::Free(kparam);
		ManagedOps::Free(dx);
		ManagedOps::Free(dy);
	}
};
#endif
