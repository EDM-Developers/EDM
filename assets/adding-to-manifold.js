const update_centered_equations = function () {
  // Read in the manifold specifications from the sliders
  const numObs = parseInt(document.getElementById("numObs").value);
  const E = parseInt(document.getElementById("E").value);
  const tau = parseInt(document.getElementById("tau").value);
  const p = 1;
  const allowMissing = false;

  // Construct the manifold and targets
  const M_x = manifold("x", numObs, E, tau, allowMissing, p).manifold;
  const M_y = manifold("y", numObs, E, tau, allowMissing, p).manifold;

  let M_x_y = M_x.map((point) => point.slice());
  let M_x_y_varying = M_x.map((point) => point.slice());
  for (let i = 0; i < M_x.length; i++) {
    M_x_y[i].push(M_y[i][0]);
    for (let j = 0; j < M_y[i].length; j++) {
      M_x_y_varying[i].push(M_y[i][j]);
    }
  }

  // Turn these into latex arrays
  const M_x_tex = latexify(M_x);
  const M_x_y_tex = latexify(M_x_y);
  const M_x_y_varying_tex = latexify(M_x_y_varying);

  // Save the result to the page
  let eqnsToTypeset = [];

  const equations = document.querySelectorAll(".dynamic-equation");
  equations.forEach((eqn) => {
    const prevRenderedEquation = eqn.dataset.renderedEquation;
    const renderedEquation = eqn.dataset.equation
      .replace(/\${M_x}/, M_x_tex)
      .replace(/\${M_x_y}/, M_x_y_tex)
      .replace(/\${M_x_y_varying}/, M_x_y_varying_tex);

    if (prevRenderedEquation != renderedEquation) {
      eqnsToTypeset.push(eqn);
      MathJax.typesetClear([eqn]);
      eqn.innerHTML = renderedEquation;
      eqn.dataset.renderedEquation = renderedEquation;
    }
  });

  MathJax.typesetPromise(eqnsToTypeset);
};

const sliders = document.querySelectorAll(".slider-container input");

sliders.forEach((slider) =>
  slider.addEventListener("input", () => update_centered_equations())
);

update_centered_equations();
